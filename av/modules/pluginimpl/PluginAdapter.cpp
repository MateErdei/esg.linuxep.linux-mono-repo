// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "PluginAdapter.h"

#include "ActionUtils.h"
#include "DetectionQueue.h"
#include "DetectionReporter.h"
#include "HealthStatus.h"
#include "Logger.h"
#include "OnAccessStatusMonitor.h"
#include "StringUtils.h"

#include "common/ApplicationPaths.h"
#include "common/PluginUtils.h"
#include "common/StringUtils.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/ZeroMQWrapper/IIPCException.h"

namespace fs = sophos_filesystem;

namespace Plugin
{
    namespace
    {
        fs::path threat_reporter_socket()
        {
            return common::getPluginInstallPath() / "chroot/var/threat_report_socket";
        }

        fs::path process_controller_socket()
        {
            return common::getPluginInstallPath() / "chroot/var/process_control_socket";
        }

        class ThreatReportCallbacks : public IMessageCallback
        {
        public:
            explicit ThreatReportCallbacks(
                PluginAdapter& adapter,
                const std::string& /* threatEventPublisherSocketPath */) :
                m_adapter(adapter)
            {
            }

            void processMessage(scan_messages::ThreatDetected detection) override
            {
                const std::string escapedPath = common::pathForLogging(detection.filePath);
                bool shouldQuarantine = true;

                if (
                    detection.reportSource == common::CentralEnums::ReportSource::ml &&
                    !m_adapter.shouldSafeStoreQuarantineMl())
                {
                    LOGINFO(escapedPath << " was not quarantined due to being reported as an ML detection");
                    shouldQuarantine = false;
                }

                try
                {
                    if (Common::FileSystem::fileSystem()->isFile(Plugin::getDisableSafestorePath()))
                    {
                        shouldQuarantine = false;
                    }
                }
                catch (Common::FileSystem::IFileSystemException &ex)
                {
                    LOGWARN("Failed to check for safestore disable path with error: " << ex.what());
                }

                // detection is not moved if the push fails, so can still be used by processDetectionReport
                if (!shouldQuarantine || !m_adapter.getDetectionQueue()->push(detection))
                {
                    if (shouldQuarantine) // i.e. failed to push
                    {
                        LOGWARN("SafeStore queue is full, unable to quarantine " << escapedPath);
                    }

                    // The quarantine is reported as a failure. Ideally we should use a more accurate reason than
                    // 'failed to delete file' but Central doesn't support anything better currently.
                    detection.quarantineResult = common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
                    m_adapter.finaliseDetection(detection);
                }
            }

        private:
            PluginAdapter& m_adapter;
        };
    } // namespace

    const PolicyWaiter::policy_list_t PluginAdapter::m_requested_policies{
        "SAV",
#ifdef ENABLE_CORC_POLICY
        "CORC",
#endif
#ifdef ENABLE_CORE_POLICY
        "CORE",
#endif
        "FLAGS",
        "ALC" // Must be last for unit tests
    };

    PluginAdapter::PluginAdapter(
        std::shared_ptr<TaskQueue> taskQueue,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback,
        const std::string& threatEventPublisherSocketPath,
        int waitForPolicyTimeout) :
        m_taskQueue(std::move(taskQueue)),
        m_detectionQueue(std::make_shared<DetectionQueue>()),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_scanScheduler(std::make_shared<manager::scheduler::ScanScheduler>(*this)),
        m_threatReporterServer(std::make_shared<unixsocket::ThreatReporterServerSocket>(
            threat_reporter_socket(),
            0660,
            std::make_shared<ThreatReportCallbacks>(*this, threatEventPublisherSocketPath))),
        m_threatDetector(std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            process_controller_socket(),
            std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>())),
        m_safeStoreWorker(std::make_shared<SafeStoreWorker>(*this, m_detectionQueue, getSafeStoreSocketPath())),
        m_restoreReportingServer{ *this },
        m_waitForPolicyTimeout(waitForPolicyTimeout),
        m_zmqContext(Common::ZMQWrapperApi::createContext()),
        m_threatEventPublisher(m_zmqContext->getPublisher()),
        m_policyProcessor(m_taskQueue),
        m_threatDatabase(Plugin::getPluginVarDirPath())
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        common::ThreadRunner sophos_threat_reporter(m_threatReporterServer, "threatReporter", true);
        LOGSUPPORT("Starting the main program loop");

        for (const std::string& policy : m_requested_policies)
        {
            try
            {
                LOGSUPPORT("Requesting " << policy << " Policy from base");
                m_baseService->requestPolicies(policy);
            }
            catch (Common::PluginApi::ApiException& e)
            {
                LOGINFO("Failed to request " << policy << " policy at startup (" << e.what() << ")");
            }
        }

        if (m_threatDatabase.isDatabaseEmpty())
        {
            m_callback->setThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);
            publishThreatHealthWithRetry(E_THREAT_HEALTH_STATUS_GOOD);
        }
        else
        {
            m_callback->setThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);
            publishThreatHealthWithRetry(E_THREAT_HEALTH_STATUS_SUSPICIOUS);
        }

        m_restoreReportingServer.start();

        innerLoop();
        LOGSUPPORT("Stopping the main program loop");
        m_schedulerThread.reset();
        m_threatDetectorThread.reset();
        // This queue blocks on pop, so must be notified
        m_detectionQueue->requestStop();
        m_safeStoreWorkerThread.reset();
        m_restoreReportingServer.requestStop();
        LOGSUPPORT("Finished the main program loop");
    }

    void PluginAdapter::startThreads()
    {
        LOGINFO("Starting Scan Scheduler");
        m_schedulerThread = std::make_unique<common::ThreadRunner>(m_scanScheduler, "scanScheduler", true);
        m_threatDetectorThread = std::make_unique<common::ThreadRunner>(m_threatDetector, "scanProcessMonitor", true);
        m_safeStoreWorkerThread = std::make_unique<common::ThreadRunner>(m_safeStoreWorker, "safeStoreWorker", true);
        connectToThreatPublishingSocket(
            Common::ApplicationConfiguration::applicationPathManager().getEventSubscriberSocketFile());
    }

    void PluginAdapter::innerLoop()
    {
        auto policyWaiter =
            std::make_shared<PolicyWaiter>(m_requested_policies, PolicyWaiter::seconds_t{ m_waitForPolicyTimeout });

        auto oaMonitor = std::make_shared<OnAccessStatusMonitor>(m_callback);
        common::ThreadRunner oaMonitorRunner{oaMonitor, "OnAccessStatusMonitor", true};

        bool threadsRunning = false;

        LOGINFO("Completed initialization of AV");
        while (true)
        {
            Task task{};
            auto timeout = std::min({ policyWaiter->timeout(), m_policyProcessor.timeout() });
            bool gotTask = m_taskQueue->pop(task, timeout);
            if (gotTask)
            {
                switch (task.taskType)
                {
                    case Task::TaskType::Stop:
                        LOGSUPPORT("Stop task received");
                        return;

                    case Task::TaskType::Policy:
                    {
                        if (task.appId == "FLAGS")
                        {
                            processFlags(task.Content);
                            policyWaiter->gotPolicy("FLAGS");
                        }
                        else
                        {
                            processPolicy(task.Content, policyWaiter, task.appId);
                        }

                        break;
                    }

                    case Task::TaskType::Action:
                        processAction(task.Content);
                        break;

                    case Task::TaskType::ScanComplete:
                        m_baseService->sendEvent("SAV", task.Content);
                        break;

                    case Task::TaskType::ThreatDetected:
                    case Task::TaskType::SendCleanEvent:
                        m_baseService->sendEvent("CORE", task.Content);
                        break;

                    case Task::TaskType::SendRestoreEvent:
                        LOGDEBUG("Sending Restore Event to Central: " << task.Content);
                        m_baseService->sendEvent("CORE", task.Content);
                        break;

                    case Task::TaskType::SendStatus:
                        m_baseService->sendStatus("SAV", task.Content, task.Content);
                        break;
                }

                processSUSIRestartRequest();
            }
            else
            {
                // timeout waiting for policy
                policyWaiter->checkTimeout();

                // or timeout to notify soapd
                m_policyProcessor.notifyOnAccessProcessIfRequired();
            }

            // if we've got policies, or timed out the initial wait then start threads
            if (!threadsRunning)
            {
                if (policyWaiter->hasAllPolicies() || policyWaiter->infoLogged())
                {
                    startThreads();
                    threadsRunning = true;
                    continue;
                }
            }
        }
    }

    void PluginAdapter::processSUSIRestartRequest()
    {
        if (m_restartSophosThreatDetector)
        {
            LOGDEBUG("Processing request to restart sophos threat detector");
            if (!m_taskQueue->queueContainsPolicyTask())
            {
                LOGDEBUG("Requesting scan monitor to restart threat detector");
                m_threatDetector->restart_threat_detector();
                m_restartSophosThreatDetector = false;
                reloadThreatDetectorConfiguration_ = false; // Already handled by restart
            }
        }
        else if (reloadThreatDetectorConfiguration_)
        {
            LOGDEBUG("Processing request to reload sophos threat detector");
            if (!m_taskQueue->queueContainsPolicyTask())
            {
                LOGDEBUG("Requesting scan monitor to reload susi");
                m_threatDetector->policy_configuration_changed();
                reloadThreatDetectorConfiguration_ = false;
            }
        }
    }

    void PluginAdapter::processFlags(const std::string& flagsJson)
    {
        LOGDEBUG("Flags Policy received: " << flagsJson);
        m_policyProcessor.processFlagSettings(flagsJson);
    }

    void PluginAdapter::processPolicy(
        const std::string& policyXml,
        const PolicyWaiterSharedPtr& policyWaiter,
        const std::string& appId)
    {
        try
        {
            if (m_currentPolicies.at(appId) == policyXml)
            {
                LOGDEBUG("Policy with app id " << appId << " unchanged, will not be processed");
                return;
            }
        }
        catch (const std::out_of_range&)
        {
            LOGDEBUG("Recieved first policy with app id " << appId);
        }

        LOGINFO("Received " << appId << " policy");
        if (policyXml.empty())
        {
            LOGERROR("Received empty policy for " << appId);
            // Can't parse empty policy
            return;
        }
        try
        {
            auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
            auto policyType = Plugin::PolicyProcessor::determinePolicyType(attributeMap, appId);
            m_currentPolicies.insert_or_assign(appId, policyXml);

            if (policyType == PolicyType::ALC)
            {
                LOGINFO("Processing ALC policy");
                LOGDEBUG("Processing ALC policy: " << policyXml);
                m_policyProcessor.processAlcPolicy(attributeMap);
                LOGDEBUG("Finished processing ALC Policy");
                policyWaiter->gotPolicy("ALC");
            }
            else if (policyType == PolicyType::SAV)
            {
                LOGINFO("Processing SAV policy");
                LOGDEBUG("Processing SAV policy: " << policyXml);
                bool policyIsValid =
                    m_scanScheduler->updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));
                if (policyIsValid)
                {
                    m_policyProcessor.processSavPolicy(attributeMap);
                    if (m_policyProcessor.restartThreatDetector())
                    {
                        m_callback->setSXL4Lookups(m_policyProcessor.getSXL4LookupsEnabled());
                    }
                    std::string revID = attributeMap.lookup("config/csc:Comp").value("RevID", "unknown");
                    m_callback->sendStatus(revID);
                    policyWaiter->gotPolicy("SAV");
                    m_threatDetector->setSXL4LookupsEnabled(m_policyProcessor.getSXL4LookupsEnabled());
                }
                else
                {
                    LOGERROR("Not processing remainder of SAV policy as Scheduled Scan configuration invalid");
                }
            }
            else if (policyType == PolicyType::CORC)
            {
                LOGINFO("Processing CORC policy");
                LOGDEBUG("Processing CORC policy: " << policyXml);
                m_policyProcessor.processCorcPolicy(attributeMap);
                policyWaiter->gotPolicy("CORC");
            }
            else if (policyType == PolicyType::CORE)
            {
                LOGINFO("Processing CORE policy");
                LOGDEBUG("Processing CORE policy: " << policyXml);
                m_policyProcessor.processCOREpolicy(attributeMap);
                policyWaiter->gotPolicy("CORE");
            }
            else if (policyType == PolicyType::UNKNOWN)
            {
                LOGDEBUG("Ignoring unknown policy with APPID: " << appId << ", content: " << policyXml);
            }
            // Check if ThreatDetector should reload SUSI config
            setReloadThreatDetector(m_policyProcessor.reloadThreatDetectorConfiguration());
            // Check if ThreatDetector should be restarted
            setResetThreatDetector(m_policyProcessor.restartThreatDetector());
        }
        catch (const Common::XmlUtilities::XmlUtilitiesException& e)
        {
            LOGERROR("Exception encountered while parsing " << appId << " policy XML: " << e.what());
        }
        catch (const std::exception& e)
        {
            LOGERROR("Exception encountered while processing " << appId << " policy: " << e.what());
        }
    }

    void PluginAdapter::processAction(const std::string& actionXml)
    {
        LOGDEBUG("Process action: " << actionXml);

        if (actionXml.front() != '<')
        {
            LOGDEBUG("Ignoring action not in XML format");
            return;
        }

        try
        {
            auto attributeMap = Common::XmlUtilities::parseXml(actionXml);

            if (pluginimpl::isScanNowAction(attributeMap))
            {
                m_scanScheduler->scanNow();
            }
            else if (pluginimpl::isSAVClearAction(attributeMap))
            {
                std::scoped_lock lock{ m_detectionMutex };
                std::string correlationID = pluginimpl::getThreatID(attributeMap);
                m_threatDatabase.removeCorrelationID(correlationID);
                if (m_threatDatabase.isDatabaseEmpty() &&
                    (m_callback->getThreatHealth() != E_THREAT_HEALTH_STATUS_GOOD))
                {
                    LOGINFO("Threat database is now empty, sending good health to Management agent");
                    publishThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);
                }
            }
            else if (pluginimpl::isCOREResetThreatHealthAction(attributeMap))
            {
                std::scoped_lock lock{ m_detectionMutex };
                LOGINFO("Resetting threat database due to core reset action");
                m_threatDatabase.resetDatabase();
                if (m_threatDatabase.isDatabaseEmpty())
                {
                    publishThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);
                }
                else
                {
                    LOGWARN("Failed to clear threat health database");
                }
            }
        }
        catch (const Common::XmlUtilities::XmlUtilitiesException& e)
        {
            LOGERROR("Exception encountered while parsing Action XML: " << e.what());
        }
        catch (const std::exception& e)
        {
            LOGERROR("Exception encountered while processing Action XML: " << e.what());
        }
    }

    void PluginAdapter::processScanComplete(std::string& scanCompletedXml)
    {
        LOGDEBUG("Sending scan complete notification to central: " << scanCompletedXml);

        m_taskQueue->push(Task{ .taskType = Task::TaskType::ScanComplete, .Content = scanCompletedXml });
    }

    void PluginAdapter::finaliseDetection(scan_messages::ThreatDetected& detection)
    {
        std::scoped_lock lock{ m_detectionMutex };

        if (m_detectionBeingQuarantined && detection.threatId == m_detectionBeingQuarantined->first)
        {
            detection.correlationId = m_detectionBeingQuarantined->second;
        }
        else if (auto correlationIdOptional = m_threatDatabase.hasThreat(detection.threatId))
        {
            detection.correlationId = correlationIdOptional.value();
        }
        else
        {
            detection.correlationId = datatypes::uuidGenerator().generate();
        }

        bool duplicate = detection.quarantineResult != common::CentralEnums::QuarantineResult::SUCCESS &&
                         m_threatDatabase.isThreatInDatabaseWithinTime(detection.threatId, DUPLICATE_DETECTION_TIMEOUT);
        if (!duplicate)
        {
            DetectionReporter::processThreatReport(pluginimpl::generateThreatDetectedXml(detection), m_taskQueue);
            DetectionReporter::publishQuarantineCleanEvent(
                pluginimpl::generateCoreCleanEventXml(detection), m_taskQueue);
            publishThreatEvent(pluginimpl::generateThreatDetectedJson(detection));
        }

        const std::string duplicateStr = duplicate ? " which is a duplicate detection" : " which is a new detection";
        std::stringstream logStr;
        logStr << "Found '" << detection.threatName << "' in '" << detection.filePath << "'" << duplicateStr;
        auto strToLog = common::escapePathForLogging(logStr.str());
        LOGDEBUG(strToLog);

        incrementTelemetryThreatCount(detection.threatName, detection.scanType);

        if (detection.quarantineResult != common::CentralEnums::QuarantineResult::SUCCESS)
        {
            m_threatDatabase.addThreat(detection.threatId, detection.correlationId);
            if (m_callback->getThreatHealth() != E_THREAT_HEALTH_STATUS_SUSPICIOUS)
            {
                LOGINFO("Threat database is not empty, sending suspicious health to Management agent");
                publishThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);
            }
        }
        else
        {
            m_threatDatabase.removeThreatID(detection.threatId);
            if (m_threatDatabase.isDatabaseEmpty())
            {
                if (m_callback->getThreatHealth() != E_THREAT_HEALTH_STATUS_GOOD)
                {
                    LOGINFO("Threat database is now empty, sending good health to Management agent");
                    publishThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);
                }
            }
        }

        if (m_detectionBeingQuarantined && m_detectionBeingQuarantined->first == detection.threatId)
        {
            // Clear as it has now either been moved to the threat database, or quarantined successfully
            m_detectionBeingQuarantined = {};
        }
    }

    void PluginAdapter::markAsQuarantining(scan_messages::ThreatDetected& detection)
    {
        std::scoped_lock lock{ m_detectionMutex };
        assert(!m_detectionBeingQuarantined); // If the precondition holds then this also must hold

        if (auto correlationIdOptional = m_threatDatabase.hasThreat(detection.threatId))
        {
            detection.correlationId = correlationIdOptional.value();
            // Already in the threat database, so don't add to m_detectionBeingQuarantined
        }
        else
        {
            detection.correlationId = datatypes::uuidGenerator().generate();
            m_detectionBeingQuarantined = { detection.threatId, detection.correlationId };
        }
    }

    void PluginAdapter::publishThreatEvent(const std::string& threatDetectedJSON) const
    {
        try
        {
            LOGDEBUG("Publishing threat detection event: " << threatDetectedJSON);
            m_threatEventPublisher->write({ "threatEvents", threatDetectedJSON });
        }
        catch (const Common::ZeroMQWrapper::IIPCException& e)
        {
            LOGERROR("Failed to send subscription data: " << e.what());
        }
    }

    void PluginAdapter::publishThreatHealth(E_HEALTH_STATUS threatStatus) const
    {
        try
        {
            LOGDEBUG("Publishing threat health: " << threatHealthToString(threatStatus));

            m_callback->setThreatHealth(threatStatus);
            m_baseService->sendThreatHealth(R"({"ThreatHealth":)" + std::to_string(threatStatus) + "}");
        }
        catch (const Common::ZeroMQWrapper::IIPCException& e)
        {
            LOGERROR("Failed to send threat health: " << e.what());
        }
        catch (Common::PluginApi::ApiException& e)
        {
            LOGWARN("Failed to send threat health: " << e.what());
        }
    }

    void PluginAdapter::publishThreatHealthWithRetry(E_HEALTH_STATUS threatStatus) const
    {
        LOGDEBUG("Publishing threat health: " << threatHealthToString(threatStatus));
        for (int i = 0; i < 5; ++i)
        {
            try
            {
                m_baseService->sendThreatHealth(R"({"ThreatHealth":)" + std::to_string(threatStatus) + "}");
                break;
            }
            catch (const Common::ZeroMQWrapper::IIPCException& e)
            {
                if (i == 4)
                {
                    LOGWARN("Failed to send threat health: " << e.what());
                }
            }
            catch (Common::PluginApi::ApiException& e)
            {
                if (i == 4)
                {
                    LOGWARN("Failed to send threat health: " << e.what());
                }
            }
            m_taskQueue->stoppableSleep(std::chrono::milliseconds(50));
        }
    }

    void PluginAdapter::incrementTelemetryThreatCount(
        const std::string& threatName,
        const scan_messages::E_SCAN_TYPE& scanType)
    {
        std::string telemetryStr = (scanType == scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS ||
                                    scanType == scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_OPEN ||
                                    scanType == scan_messages::E_SCAN_TYPE::E_SCAN_TYPE_ON_ACCESS_CLOSE)
                                       ? "on-access-"
                                       : "on-demand-";

        telemetryStr.append((threatName == "EICAR-AV-Test") ? "threat-eicar-count" : "threat-count");

        Common::Telemetry::TelemetryHelper::getInstance().increment(telemetryStr, 1ul);
    }

    void PluginAdapter::connectToThreatPublishingSocket(const std::string& pubSubSocketAddress)
    {
        m_threatEventPublisher->connect("ipc://" + pubSubSocketAddress);
    }

    bool PluginAdapter::shouldSafeStoreQuarantineMl() const
    {
        return m_policyProcessor.shouldSafeStoreQuarantineMl();
    }

    std::shared_ptr<DetectionQueue> PluginAdapter::getDetectionQueue() const
    {
        return m_detectionQueue;
    }

    void PluginAdapter::processRestoreReport(const scan_messages::RestoreReport& restoreReport) const
    {
        const std::string escapedPath = common::escapePathForLogging(restoreReport.path);
        LOGINFO(
            "Reporting " << (restoreReport.wasSuccessful ? "successful" : "unsuccessful") << " restoration of "
                         << escapedPath);
        m_taskQueue->push(Task{ .taskType = Task::TaskType::SendRestoreEvent,
                                .Content = pluginimpl::generateCoreRestoreEventXml(restoreReport) });
        LOGDEBUG("Added restore report to task queue");
    }
} // namespace Plugin