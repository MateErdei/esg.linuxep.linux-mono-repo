/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "Logger.h"
#include "Telemetry.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

using namespace Plugin;

PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_scanScheduler(*this)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophos_threat_detector_path = appConfig.getData("PLUGIN_INSTALL");
    sophos_threat_detector_path /= "sbin/sophos_threat_detector_launcher";
    m_sophosThreadDetector = std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(
            sophos_threat_detector_path
    );
}

namespace
{
    class ThreadRunner
    {
    public:
        explicit ThreadRunner(Common::Threads::AbstractThread& thread, std::string name)
                : m_thread(thread), m_name(std::move(name))
        {
            LOGINFO("Starting " << name);
            m_thread.start();
        }

        ~ThreadRunner()
        {
            LOGINFO("Stopping " << m_name);
            m_thread.requestStop();
            LOGINFO("Joining " << m_name);
            m_thread.join();
        }

    private:
        Common::Threads::AbstractThread& m_thread;
        std::string m_name;
    };
}

void PluginAdapter::mainLoop()
{
    LOGINFO("Entering the main loop");
    ThreadRunner scheduler(m_scanScheduler, "scanScheduler"); // Automatically terminate scheduler on both normal exit and exceptions
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
                m_baseService->sendEvent("2", task.Content);
                break;
        }
    }
}

void PluginAdapter::processPolicy(const std::string& policyXml)
{
    LOGDEBUG("Process policy: " << policyXml);

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    m_scanScheduler.updateConfig(manager::scheduler::ScheduledScanConfiguration(attributeMap));
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
    LOGDEBUG("Sending scan complete notification to central " << scanCompletedXml);

    m_queueTask->push(Task{.taskType=Task::TaskType::ScanComplete, scanCompletedXml});
}
