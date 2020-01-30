/******************************************************************************************************

Copyright 2018-2019 Sophos Limited.  All rights reserved.

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
    m_callback(std::move(callback))
{

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path sophos_threat_detector_path = appConfig.getData("PLUGIN_INSTALL");
    sophos_threat_detector_path /= "sbin/sophos_threat_detector";
    m_sophosThreadDetector = std::make_unique<plugin::manager::scanprocessmonitor::ScanProcessMonitor>(sophos_threat_detector_path);
}

namespace
{
    class ThreadRunner
    {
    public:
        explicit ThreadRunner(Common::Threads::AbstractThread& thread)
            : m_thread(thread)
        {
            m_thread.start();
        }
        ~ThreadRunner()
        {
            m_thread.requestStop();
            m_thread.join();
        }

    private:
        Common::Threads::AbstractThread& m_thread;
    };
}

void PluginAdapter::mainLoop()
{
    LOGINFO("Entering the main loop");
    ThreadRunner scheduler(m_scanScheduler); // Automatically terminate scheduler on both normal exit and exceptions
    ThreadRunner sophos_thread_detector(*m_sophosThreadDetector);
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
