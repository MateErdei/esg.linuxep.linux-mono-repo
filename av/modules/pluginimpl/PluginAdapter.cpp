/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "Logger.h"
#include "StringUtils.h"

#include "datatypes/sophos_filesystem.h"
#include "modules/common/ThreadRunner.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/ZeroMQWrapper/IIPCException.h>

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
            explicit ThreatReportCallbacks(PluginAdapter& adapter, const std::string& threatEventPublisherSocketPath)
                : m_adapter(adapter)
                , m_threatEventPublisherSocketPath(threatEventPublisherSocketPath)
            {}

            void processMessage(const scan_messages::ServerThreatDetected& detection) override
            {
                m_adapter.processThreatReport(pluginimpl::generateThreatDetectedXml(detection));
                m_adapter.publishThreatEvent(
                    pluginimpl::generateThreatDetectedJson(detection.getThreatName(), detection.getFilePath()),
                    m_threatEventPublisherSocketPath);
            }

        private:
            PluginAdapter& m_adapter;
            std::string m_threatEventPublisherSocketPath;
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
        m_threatReporterServer(threat_reporter_socket(), 0600,
                               std::make_shared<ThreatReportCallbacks>(*this, threatEventPublisherSocketPath)),
        m_threatDetector(std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            sophos_threat_detector_launcher())),
        m_waitForPolicyTimeout(waitForPolicyTimeout),
        m_zmqContext(Common::ZMQWrapperApi::createContext()),
        m_threatEventPublisher(m_zmqContext->getPublisher())
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

        auto policySAVXml = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(m_waitForPolicyTimeout), 5, "2", "SAV");
        if (!policySAVXml.empty())
        {
            processPolicy(policySAVXml);
        }

        auto policyALCXml = waitForTheFirstPolicy(*m_queueTask, std::chrono::seconds(m_waitForPolicyTimeout), 5, "1", "ALC");
        if (!policyALCXml.empty())
        {
            processPolicy(policyALCXml);
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
                // ALC policy
                LOGINFO("Processing ALC Policy");
                LOGDEBUG("Processing policy: " << policyXml);
                bool updated = m_policyProcessor.processAlcPolicy(attributeMap);
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

        // SAV policy
        LOGINFO("Processing SAV Policy");
        LOGDEBUG("Processing policy: " << policyXml);

        m_scanScheduler.updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));

        auto savPolicyHasChanged = m_policyProcessor.processSavPolicy(attributeMap);
        if (savPolicyHasChanged)
        {
            LOGDEBUG("Reloading susi as startup configuration changed");
            m_threatDetector->configuration_changed();
        }

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
        std::string tempPolicyXml;

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
                tempPolicyXml = task.Content;
                auto attributeMap = Common::XmlUtilities::parseXml(tempPolicyXml);

                auto alc_comp = attributeMap.lookup("AUConfigurations/csc:Comp");
                //returns unknown if it's not ALC type
                auto policyType = alc_comp.value("policyType", "unknown");

                if (policyType == "unknown")
                {
                    //returns unknown if it's not SAV type
                    policyType = attributeMap.lookup("config/csc:Comp").value("policyType", "unknown");
                }

                if (policyAppId == policyType)
                {
                    LOGINFO(policyName << " policy received for the first time.");
                    policyXml = tempPolicyXml;
                    break;
                }
            }

            LOGSUPPORT("Keep task: " << task.getTaskName());
            nonPolicyTasks.push_back(task);
        }

        LOGDEBUG("Return from waitForTheFirstPolicy");

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

    void PluginAdapter::publishThreatEvent(const std::string& threatDetectedJSON, const std::string& threatEventPublisherSocketPath)
    {
        try
        {
            m_threatEventPublisher->connect("ipc://" + threatEventPublisherSocketPath);
            LOGDEBUG("Publishing threat detection event: " << threatDetectedJSON);
            m_threatEventPublisher->write({ "threatEvents", threatDetectedJSON });
        }
        catch (const Common::ZeroMQWrapper::IIPCException& e)
        {
            LOGERROR("Failed to send subscription data: " << e.what());
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
}