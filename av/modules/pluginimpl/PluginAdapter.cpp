// Copyright 2018-2022, Sophos Limited.  All rights reserved.

# define PLUGIN_INTERNAL public

#include "PluginAdapter.h"

#include "DetectionQueue.h"
#include "DetectionReporter.h"
#include "HealthStatus.h"
#include "Logger.h"
#include "PolicyWaiter.h"
#include "StringUtils.h"

#include "datatypes/SystemCallWrapper.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "Common/ZeroMQWrapper/IIPCException.h"
#include "common/PluginUtils.h"
#include "common/ApplicationPaths.h"

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
            explicit ThreatReportCallbacks(PluginAdapter& adapter, const std::string& /* threatEventPublisherSocketPath */)
                : m_adapter(adapter)
            {}

            void processMessage(scan_messages::ThreatDetected detection) override
            {
                // detection is not moved if the push fails, so can still be used by processDetectionReport
                if (!m_adapter.isSafeStoreEnabled() || !m_adapter.getDetectionQueue()->push(detection))
                {
                    // TODO: LINUXDAR-5677 - Modify report to include no quarantine happened
                    // reportNoQuarantine(detection);
                    m_adapter.processDetectionReport(detection);
                }
            }

        private:
            PluginAdapter& m_adapter;
        };
    }

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
        m_threatReporterServer(std::make_shared<unixsocket::ThreatReporterServerSocket>(threat_reporter_socket(), 0660,
                               std::make_shared<ThreatReportCallbacks>(*this, threatEventPublisherSocketPath))),
        m_threatDetector(std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            process_controller_socket(), std::make_shared<datatypes::SystemCallWrapper>())),
        m_safeStoreWorker(std::make_shared<SafeStoreWorker>(*this, m_detectionQueue, getSafeStoreSocketPath())),
        m_waitForPolicyTimeout(waitForPolicyTimeout),
        m_zmqContext(Common::ZMQWrapperApi::createContext()),
        m_threatEventPublisher(m_zmqContext->getPublisher())
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        common::ThreadRunner sophos_threat_reporter(m_threatReporterServer, "threatReporter", true);
        LOGSUPPORT("Starting the main program loop");
        try
        {
            LOGSUPPORT("Requesting SAV Policy from base");
            m_baseService->requestPolicies("SAV");
        }
        catch (Common::PluginApi::ApiException& e)
        {
            LOGINFO("Failed to request SAV policy at startup (" << e.what() << ")");
        }
        try
        {
            LOGSUPPORT("Requesting ALC Policy from base");
            m_baseService->requestPolicies("ALC");
        }
        catch (Common::PluginApi::ApiException& e)
        {
            LOGINFO("Failed to request ALC policy at startup (" << e.what() << ")");
        }
        try
        {
            m_baseService->requestPolicies("FLAGS");
        }
        catch (Common::PluginApi::ApiException& e)
        {
            LOGINFO("Failed to request FLAGS policy at startup (" << e.what() << ")");
        }

        m_callback->setThreatHealth(
            static_cast<E_HEALTH_STATUS>(pluginimpl::getThreatStatus())
            );

        innerLoop();
        LOGSUPPORT("Stopping the main program loop");
        m_schedulerThread.reset();
        m_threatDetectorThread.reset();
        // This queue blocks on pop, so must be notified
        m_detectionQueue->requestStop();
        m_safeStoreWorkerThread.reset();
        LOGSUPPORT("Finished the main program loop");
    }

    void PluginAdapter::startThreads()
    {
        m_schedulerThread = std::make_unique<common::ThreadRunner>(m_scanScheduler, "scanScheduler", true);
        m_threatDetectorThread = std::make_unique<common::ThreadRunner>(m_threatDetector, "scanProcessMonitor", true);
        m_safeStoreWorkerThread = std::make_unique<common::ThreadRunner>(m_safeStoreWorker, "safeStoreWorker", true);
        connectToThreatPublishingSocket(
            Common::ApplicationConfiguration::applicationPathManager().getEventSubscriberSocketFile());
    }

    void PluginAdapter::innerLoop()
    {
        PolicyWaiter::policy_list_t policies;
        policies.push_back("SAV");
        policies.push_back("ALC");
        PolicyWaiter policyWaiter(std::move(policies), PolicyWaiter::seconds_t{m_waitForPolicyTimeout});

        bool threadsRunning = false;

        while (true)
        {
            Task task{};
            bool gotTask = m_taskQueue->pop(task, policyWaiter.timeout());
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
                        }
                        else
                        {
                            bool policyUpdated = false;
                            std::string appId;
                            processPolicy(task.Content, policyUpdated, appId);
                            policyWaiter.gotPolicy(appId);
                            m_restartSophosThreatDetector = policyUpdated || m_restartSophosThreatDetector;
                        }

                        break;
                    }

                    case Task::TaskType::Action:
                        processAction(task.Content);
                        break;

                    case Task::TaskType::ScanComplete:
                    case Task::TaskType::ThreatDetected:
                        m_baseService->sendEvent("SAV", task.Content);
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
                policyWaiter.checkTimeout();
            }

            // if we've got policies, or timed out the initial wait then start threads
            if (!threadsRunning)
            {
                if (policyWaiter.hasAllPolicies() || policyWaiter.infoLogged())
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
            if(!m_taskQueue->queueContainsPolicyTask())
            {
                LOGDEBUG("Requesting scan monitor to reload susi");
                m_threatDetector->policy_configuration_changed();
                m_restartSophosThreatDetector = false;
            }
        }
    }

    void PluginAdapter::processFlags(const std::string& flagsJson)
    {
        LOGDEBUG("Flags Policy received: " << flagsJson);
        m_policyProcessor.processFlagSettings(flagsJson);

        m_callback->setSafeStoreEnabled(m_policyProcessor.isSafeStoreEnabled());
    }

    void PluginAdapter::processPolicy(const std::string& policyXml, bool& policyUpdated, std::string& appId)
    {
        LOGINFO("Received Policy");
        auto attributeMap = Common::XmlUtilities::parseXml(policyXml);

        // Work out whether it's ALC or SAV policy
        auto alc_comp = attributeMap.lookup("AUConfigurations/csc:Comp");
        auto policyType = alc_comp.value("policyType", "unknown");
        if (policyType != "unknown")
        {
            if (policyType == "1")
            {
                // ALC policy
                LOGINFO("Processing ALC Policy");
                LOGDEBUG("Processing policy: " << policyXml);
                bool updated = m_policyProcessor.processAlcPolicy(attributeMap);
                if (!m_gotAlcPolicy)
                {
                    LOGINFO("ALC policy received for the first time.");
                    m_gotAlcPolicy = true;
                }
                policyUpdated = updated;
                appId = "ALC";
                return;
            }
            else
            {
                LOGDEBUG("Ignoring policy of incorrect type: " << policyType);
            }
            policyUpdated = false;
            appId = "";
            return;
        }

        // SAV policy
        policyType = attributeMap.lookup("config/csc:Comp").value("policyType", "unknown");
        if (policyType != "2")
        {
            LOGDEBUG("Ignoring policy of incorrect type: " << policyType);
            policyUpdated = false;
            appId = "";
            return;
        }

        LOGINFO("Processing SAV Policy");
        LOGDEBUG("Processing policy: " << policyXml);

        bool savPolicyHasChanged = m_policyProcessor.processSavPolicy(attributeMap, m_gotSavPolicy);
        m_callback->setSXL4Lookups(m_policyProcessor.getSXL4LookupsEnabled());
        m_scanScheduler->updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));

        std::string revID = attributeMap.lookup("config/csc:Comp").value("RevID", "unknown");
        m_callback->sendStatus(revID);
        if (!m_gotSavPolicy)
        {
            LOGINFO("SAV policy received for the first time.");
            m_gotSavPolicy = true;
        }
        policyUpdated = savPolicyHasChanged;
        appId = "SAV";
    }

    void PluginAdapter::processAction(const std::string& actionXml)
    {
        LOGDEBUG("Process action: " << actionXml);

        auto attributeMap = Common::XmlUtilities::parseXml(actionXml);

        if (attributeMap.lookup("a:action").value("type", "") == "ScanNow")
        {
            m_scanScheduler->scanNow();
        }
    }

    void PluginAdapter::processScanComplete(std::string& scanCompletedXml, int exitCode)
    {
        if (( exitCode == common::E_CLEAN_SUCCESS ||  exitCode == common::E_GENERIC_FAILURE ||  exitCode == common::E_PASSWORD_PROTECTED ))
        {
            LOGDEBUG("Publishing good threat health status after clean scan");
            publishThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);
        }

        LOGDEBUG("Sending scan complete notification to central: " << scanCompletedXml);

        m_taskQueue->push(Task { .taskType = Task::TaskType::ScanComplete, .Content = scanCompletedXml });
    }

    void PluginAdapter::processDetectionReport(const scan_messages::ThreatDetected& detection) const
    {
        DetectionReporter::processThreatReport(pluginimpl::generateThreatDetectedXml(detection), m_taskQueue);
        publishThreatEvent(pluginimpl::generateThreatDetectedJson(detection));
        publishThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);
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
            m_baseService->sendThreatHealth("{\"ThreatHealth\":" + std::to_string(threatStatus) + "}");
        }
        catch (const Common::ZeroMQWrapper::IIPCException& e)
        {
            LOGERROR("Failed to send threat health: " << e.what());
        }
    }

    void PluginAdapter::incrementTelemetryThreatCount(const std::string& threatName)
    {
        if (threatName == "EICAR-AV-Test")
        {
            Common::Telemetry::TelemetryHelper::getInstance().increment("threat-eicar-count", 1ul);
        }
        else
        {
            Common::Telemetry::TelemetryHelper::getInstance().increment("threat-count", 1ul);
        }
    }

    void PluginAdapter::connectToThreatPublishingSocket(const std::string& pubSubSocketAddress)
    {
        m_threatEventPublisher->connect("ipc://" + pubSubSocketAddress);
    }

    bool PluginAdapter::isSafeStoreEnabled()
    {
        return m_policyProcessor.isSafeStoreEnabled();
    }

    std::shared_ptr<DetectionQueue> PluginAdapter::getDetectionQueue() const
    {
        return m_detectionQueue;
    }
}