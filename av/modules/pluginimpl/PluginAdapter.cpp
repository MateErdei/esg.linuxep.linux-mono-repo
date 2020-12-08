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

using namespace Plugin;

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
        return pluginInstall() /  "sbin/sophos_threat_detector_launcher";
    }

    class ThreatReportCallbacks : public IMessageCallback
    {
    public:
        explicit ThreatReportCallbacks(PluginAdapter& adapter)
                :m_adapter(adapter)
        {

        }

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
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_scanScheduler(*this),
        m_threatReporterServer(threat_reporter_socket(), 0600, std::make_shared<ThreatReportCallbacks>(*this)),
        m_sophosThreadDetector(std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            sophos_threat_detector_launcher()))
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
    ThreadRunner scheduler(m_scanScheduler, "scanScheduler"); // Automatically terminate scheduler on both normal exit and exceptions
    ThreadRunner sophos_threat_reporter(m_threatReporterServer, "threatReporter");
    ThreadRunner sophos_thread_detector(*m_sophosThreadDetector, "threatDetector");
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
    LOGDEBUG("Processing policy: " << policyXml);

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);

    // Work out whether it's ALC or SAV policy
    auto alc_comp = attributeMap.lookup("AUConfigurations/csc:Comp");
    auto policyType = alc_comp.value("policyType", "unknown");
    if (policyType == "1")
    {
        // ALC policy
        bool updated = m_updatePolicyProcessor.processAlcPolicy(attributeMap);
        if (updated)
        {
            m_sophosThreadDetector->configuration_changed();
        }
        return;
    }

    // SAV policy
    policyType = attributeMap.lookup("config/csc:Comp").value("policyType", "unknown");
    if ( policyType != "2")
    {
        LOGDEBUG("Ignoring policy of incorrect type: " << policyType);
        return;
    }
    m_scanScheduler.updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));

    std::string revID = attributeMap.lookup("config/csc:Comp").value("RevID", "unknown");
    m_callback->sendStatus(revID);
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

    m_queueTask->push(Task{.taskType=Task::TaskType::ScanComplete, .Content=scanCompletedXml});
}

void PluginAdapter::processThreatReport(const std::string& threatDetectedXML)
{
    LOGDEBUG("Sending threat detection notification to central: " << threatDetectedXML);
    auto attributeMap = Common::XmlUtilities::parseXml(threatDetectedXML);
    incrementTelemetryThreatCount(attributeMap.lookupMultiple("notification/threat")[0].value("name"));
    m_queueTask->push(Task{.taskType=Task::TaskType::ThreatDetected, .Content=threatDetectedXML});
}

void PluginAdapter::incrementTelemetryThreatCount(const std::string& threatName)
{
    if(threatName == "EICAR-AV-Test")
    {
        Common::Telemetry::TelemetryHelper::getInstance().increment("threat-eicar-count", 1ul);
    }
    else
    {
        Common::Telemetry::TelemetryHelper::getInstance().increment("threat-count", 1ul);
    }
}