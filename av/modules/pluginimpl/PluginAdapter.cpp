/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "modules/common/ThreadRunner.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>


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
        return pluginInstall() / "chroot/threat_report_socket";
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
    try
    {
        m_baseService->requestPolicies("SAV");
    }
    catch (std::exception& e)
    {
        LOGERROR("Failed to get SAV policy at startup");
    }
}

void PluginAdapter::mainLoop()
{
    LOGSUPPORT("Starting the main program loop");
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
    LOGDEBUG("Process policy: " << policyXml);

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    std::string policyType = attributeMap.lookup("config/csc:Comp").value("policyType", "unknown");
    if ( policyType != "2")
    {
        LOGDEBUG("Ignoring policy of incorrect type");
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

    m_queueTask->push(Task{.taskType=Task::TaskType::ScanComplete, scanCompletedXml});
}

void PluginAdapter::processThreatReport(const std::string& threatDetectedXML)
{
    LOGDEBUG("Sending threat detection notification to central: " << threatDetectedXML);

    m_queueTask->push(Task{.taskType=Task::TaskType::ThreatDetected, threatDetectedXML});
}
