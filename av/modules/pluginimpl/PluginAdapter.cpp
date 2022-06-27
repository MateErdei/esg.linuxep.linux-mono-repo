/******************************************************************************************************

Copyright 2018-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "HealthStatus.h"
#include "Logger.h"
#include "StringUtils.h"

#include "common/PluginUtils.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/SystemCallWrapper.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
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
            explicit ThreatReportCallbacks(PluginAdapter& adapter, const std::string& /* threatEventPublisherSocketPath */)
                : m_adapter(adapter)
            {}

            void processMessage(const scan_messages::ServerThreatDetected& detection) override
            {
                m_adapter.processThreatReport(pluginimpl::generateThreatDetectedXml(detection));
                m_adapter.publishThreatEvent(pluginimpl::generateThreatDetectedJson(detection));
                m_adapter.publishThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);
            }

        private:
            PluginAdapter& m_adapter;
        };
    }

    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback,
        const std::string& threatEventPublisherSocketPath,
        int waitForPolicyTimeout) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_scanScheduler(*this),
        m_threatReporterServer(threat_reporter_socket(), 0660,
                               std::make_shared<ThreatReportCallbacks>(*this, threatEventPublisherSocketPath)),
        m_threatDetector(std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            process_controller_socket(), std::make_shared<datatypes::SystemCallWrapper>())),
        m_waitForPolicyTimeout(waitForPolicyTimeout),
        m_zmqContext(Common::ZMQWrapperApi::createContext()),
        m_threatEventPublisher(m_zmqContext->getPublisher())
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        common::ThreadRunner sophos_threat_reporter(m_threatReporterServer, "threatReporter");
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

        m_callback->setThreatHealth(pluginimpl::getThreatStatus());

        innerLoop();
        LOGSUPPORT("Stopping the main program loop");
    }

    void PluginAdapter::startThreads()
    {
        m_schedulerThread = std::make_unique<common::ThreadRunner>(m_scanScheduler, "scanScheduler");
        m_threatDetectorThread = std::make_unique<common::ThreadRunner>(*m_threatDetector, "scanProcessMonitor");
        connectToThreatPublishingSocket(
            Common::ApplicationConfiguration::applicationPathManager().getEventSubscriberSocketFile());
    }

    void PluginAdapter::innerLoop()
    {
        auto initialPolicyDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(m_waitForPolicyTimeout);
        bool threadsRunning = false;

        while (true)
        {
            Task task;
            if (!threadsRunning)
            {
                if ( m_gotSavPolicy && m_gotAlcPolicy )
                {
                    startThreads();
                    threadsRunning = true;
                    continue;
                }
                if (!m_queueTask->pop(task, initialPolicyDeadline))
                {
                    // timed out
                    if (!m_gotSavPolicy)
                    {
                        LOGINFO("SAV policy has not been sent to the plugin");
                    }
                    if (!m_gotAlcPolicy)
                    {
                        LOGINFO("ALC policy has not been sent to the plugin");
                    }

                    startThreads();
                    threadsRunning = true;
                    continue;
                }
            }
            else
            {
                task = m_queueTask->pop();
            }

            switch (task.taskType)
            {
                case Task::TaskType::Stop:
                    return;

                case Task::TaskType::Policy:
                    m_restartSophosThreatDetector = processPolicy(task.Content) || m_restartSophosThreatDetector;
                    break;

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
    }

    void PluginAdapter::processSUSIRestartRequest()
    {
        if (m_restartSophosThreatDetector)
        {
            LOGDEBUG("Processing request to restart sophos threat detector");
            if(!m_queueTask->queueContainsPolicyTask())
            {
                LOGDEBUG("Requesting scan monitor to reload susi");
                m_threatDetector->policy_configuration_changed();
                m_restartSophosThreatDetector = false;
            }
        }
    }

    bool PluginAdapter::processPolicy(const std::string& policyXml)
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
                return updated;
            }
            else
            {
                LOGDEBUG("Ignoring policy of incorrect type: " << policyType);
            }
            return false;
        }

        // SAV policy
        policyType = attributeMap.lookup("config/csc:Comp").value("policyType", "unknown");
        if (policyType != "2")
        {
            LOGDEBUG("Ignoring policy of incorrect type: " << policyType);
            return false;
        }

        LOGINFO("Processing SAV Policy");
        LOGDEBUG("Processing policy: " << policyXml);

        bool savPolicyHasChanged = m_policyProcessor.processSavPolicy(attributeMap, m_gotSavPolicy);
        m_callback->setSXL4Lookups(m_policyProcessor.getSXL4LookupsEnabled());
        m_scanScheduler.updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));

        std::string revID = attributeMap.lookup("config/csc:Comp").value("RevID", "unknown");
        m_callback->sendStatus(revID);
        if (!m_gotSavPolicy)
        {
            LOGINFO("SAV policy received for the first time.");
            m_gotSavPolicy = true;
        }
        return savPolicyHasChanged;
    }

    void PluginAdapter::processAction(const std::string& actionXml)
    {
        LOGDEBUG("Process action: " << actionXml);

        auto attributeMap = Common::XmlUtilities::parseXml(actionXml);

        if (attributeMap.lookup("a:action").value("type", "") == "ScanNow")
        {
            m_scanScheduler.scanNow();
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

        m_queueTask->push(Task { .taskType = Task::TaskType::ScanComplete, .Content = scanCompletedXml });
    }

    void PluginAdapter::processThreatReport(const std::string& threatDetectedXML)
    {
        LOGDEBUG("Sending threat detection notification to central: " << threatDetectedXML);
        auto attributeMap = Common::XmlUtilities::parseXml(threatDetectedXML);
        incrementTelemetryThreatCount(attributeMap.lookupMultiple("notification/threat")[0].value("name"));
        m_queueTask->push(Task { .taskType = Task::TaskType::ThreatDetected, .Content = threatDetectedXML });
    }

    void PluginAdapter::publishThreatEvent(const std::string& threatDetectedJSON)
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

    void PluginAdapter::publishThreatHealth(long threatStatus)
    {
        try
        {
            LOGDEBUG("Publishing threat health: " << threatStatus << " (1 = good, 2 = suspicious)");

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
}