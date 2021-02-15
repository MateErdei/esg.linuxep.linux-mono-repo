/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "modules/common/ThreadRunner.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

namespace fs = sophos_filesystem;

namespace Plugin
{
    namespace
    {
        fs::path pluginInstall()
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            return appConfig.getData("PLUGIN_INSTALL");
        }

        fs::path threat_reporter_socket()
        {
            return pluginInstall() / "chroot/var/threat_report_socket";
        }

        fs::path sophos_threat_detector_launcher()
        {
            return pluginInstall() / "sbin/sophos_threat_detector_launcher";
        }

        class ThreatReportCallbacks : public IMessageCallback
        {
        public:
            explicit ThreatReportCallbacks(PluginAdapter& adapter) : m_adapter(adapter) {}

            void processMessage(const std::string& threatXML) override
            {
                m_adapter.processThreatReport(threatXML);
            }

        private:
            PluginAdapter& m_adapter;
        };
    }

    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback,
        int waitForPolicyTimeout) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_scanScheduler(*this),
        m_threatReporterServer(threat_reporter_socket(), 0600, std::make_shared<ThreatReportCallbacks>(*this)),
        m_threatDetector(std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            sophos_threat_detector_launcher())),
        m_waitForPolicyTimeout(waitForPolicyTimeout)
    {
    }

    void PluginAdapter::mainLoop()
    {
        LOGSUPPORT("Starting the main program loop");
        try
        {
            LOGSUPPORT("Requesting SAV Policy from base");
            m_baseService->requestPolicies("SAV");
        }
        catch (Common::PluginApi::ApiException& e)
        {
            LOGERROR("Failed to get SAV policy at startup (" << e.what() << ")");
        }
        try
        {
            LOGSUPPORT("Requesting ALC Policy from base");
            m_baseService->requestPolicies("ALC");
        }
        catch (Common::PluginApi::ApiException& e)
        {
            LOGERROR("Failed to get ALC policy at startup (" << e.what() << ")");
        }

        auto policySAVXml = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(m_second), 5, "2", "SAV");
        if (!policySAVXml.empty())
        {
            processPolicy(policySAVXml);
            m_processedSAVPolicyBeforeLoop = true;
        }

        auto policyALCXml = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(m_second), 5, "1", "ALC");
        if (!policyALCXml.empty())
        {
            processPolicy(policyALCXml);
            m_processedALCPolicyBeforeLoop = true;
        }

        ThreadRunner scheduler(
            m_scanScheduler, "scanScheduler"); // Automatically terminate scheduler on both normal exit and exceptions
        ThreadRunner sophos_threat_reporter(m_threatReporterServer, "threatReporter");
        ThreadRunner sophos_thread_detector(*m_threatDetector, "threatDetector");
        innerLoop();
    }

    void PluginAdapter::innerLoop()
    {
        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.taskType)
            {
                case Task::TaskType::Stop:
                    return;

                case Task::TaskType::Policy:
                    processPolicy(task.Content);
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
        }
    }

    void PluginAdapter::processPolicy(const std::string& policyXml)
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
                if (m_processedALCPolicyBeforeLoop)
                {
                    m_processedALCPolicyBeforeLoop = false;
                    LOGINFO("ALC Policy already processed");
                    return;
                }

                // ALC policy
                LOGINFO("Processing ALC Policy");
                LOGDEBUG("Processing policy: " << policyXml);
                bool updated = m_updatePolicyProcessor.processAlcPolicy(attributeMap);
                if (updated)
                {
                    m_threatDetector->configuration_changed();
                }
            }
            else
            {
                LOGDEBUG("Ignoring policy of incorrect type: " << policyType);
            }
            return;
        }

        // SAV policy
        policyType = attributeMap.lookup("config/csc:Comp").value("policyType", "unknown");
        if (policyType != "2")
        {
            LOGDEBUG("Ignoring policy of incorrect type: " << policyType);
            return;
        }

        if (m_processedSAVPolicyBeforeLoop)
        {
            m_processedSAVPolicyBeforeLoop = false;
            LOGINFO("SAV Policy already processed");
            return;
        }

        LOGINFO("Processing SAV Policy");
        LOGDEBUG("Processing policy: " << policyXml);

        m_scanScheduler.updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));

        std::string revID = attributeMap.lookup("config/csc:Comp").value("RevID", "unknown");
        m_callback->sendStatus(revID);
    }

    std::string PluginAdapter::waitForTheFirstPolicy(QueueTask& queueTask, std::chrono::seconds timeoutInS,
                                                    int maxTasksThreshold,
                                                    const std::string& policyAppId, const std::string& policyName)
    {
        std::vector<Plugin::Task> nonPolicyTasks;
        std::string policyXml;
        for (int i = 0; i < maxTasksThreshold; i++)
        {
            Plugin::Task task;

            if (!queueTask.pop(task, timeoutInS.count()))
            {
                LOGINFO(policyName << " policy has not been sent to the plugin");
                break;
            }

            if (task.taskType == Plugin::Task::TaskType::Policy)
            {
                policyXml = task.Content;
                auto attributeMap = Common::XmlUtilities::parseXml(policyXml);

                auto alc_comp = attributeMap.lookup("AUConfigurations/csc:Comp");
                auto policyType = alc_comp.value("policyType", "unknown");

                if (policyType == "unknown")
                {
                    policyType = attributeMap.lookup("config/csc:Comp").value("policyType", "unknown");
                }

                if (policyType == "unknown")
                {
                    policyXml = "";
                }

                if (policyAppId == policyType)
                {
                    LOGINFO(policyName << " policy received for the first time.");
                    break;
                }
            }

            LOGSUPPORT("Keep task: " << task.getTaskName());
            nonPolicyTasks.push_back(task);
        }

        LOGINFO("Return from waitForTheFirstPolicy");

        for (const auto& task : nonPolicyTasks)
        {
            queueTask.push(task);
        }

        return policyXml;
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

    void PluginAdapter::processScanComplete(std::string& scanCompletedXml)
    {
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
}