// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginAdapter.h"

#include "ActionProcessor.h"
#include "ApplicationPaths.h"
#include "Logger.h"

#include "Common/UtilityImpl/TimeUtils.h"

#include <utility>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
            std::shared_ptr<TaskQueue> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback)
            : m_taskQueue(std::move(queueTask))
            , m_baseService(std::move(baseService))
            , m_callback(std::move(callback))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");
        while (true)
        {
            Task task;
            if (!m_taskQueue->pop(task, QUEUE_TIMEOUT))
            {
                LOGDEBUG("Timed out waiting for task");
            }
            else
            {
                switch (task.taskType)
                {
                    case Task::TaskType::Stop:
                        return;
                    case Task::TaskType::Policy:
                        LOGERROR("Received unexpected Policy Task");
                        break;
                    case Task::TaskType::Action:
                        processAction(task.Content);
                        break;
                }
            }
        }
    }

    void PluginAdapter::processAction(const std::string& actionXml)
    {
        LOGDEBUG("Received action: " << actionXml);
        auto action = ActionProcessor::processIsolateAction(actionXml);
        if (!action)
        {
            // Error logged in processIsolateAction
            LOGDEBUG("Ignoring invalid action: " << actionXml);
            return;
        }
        if (action.value())
        {
            LOGINFO("Enabling Device Isolation");
        }
        else
        {
            LOGINFO("Disabling Device Isolation");
        }
    }

} // namespace Plugin