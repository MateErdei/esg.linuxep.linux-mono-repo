/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerProcessor.h"

#include "SchedulerTask.h"

namespace TelemetrySchedulerImpl
{
    SchedulerProcessor::SchedulerProcessor(
        std::shared_ptr<TaskQueue> taskQueue,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> pluginCallback) :
        m_taskQueue(std::move(taskQueue)),
        m_baseService(std::move(baseService)),
        m_pluginCallback(std::move(pluginCallback))
    {
    }

    void SchedulerProcessor::run()
    {
        while (true)
        {
            auto task = m_taskQueue->pop();

            switch (task.taskType)
            {
                case Task::TaskType::ShutdownReceived:
                    continue;

                default:
                    throw std::logic_error("unexpected task type");
            }
        }
    }
} // namespace TelemetrySchedulerImpl
