// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "PluginAdapter.h"

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");

        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.m_taskType)
            {
                case Task::TaskType::STOP:
                    return;
                case Task::TaskType::ACTION:
                    processAction(task.m_content);
                    break;

            }
        }
    }

    void PluginAdapter::processAction(const std::string& actionXml) { LOGDEBUG("Process policy: " << actionXml); }

} // namespace Plugin