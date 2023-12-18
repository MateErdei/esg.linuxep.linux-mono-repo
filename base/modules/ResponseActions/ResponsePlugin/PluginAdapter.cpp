// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginAdapter.h"
#include "Logger.h"
#include "PluginUtils.h"

#ifdef SPL_BAZEL
#include "AutoVersioningHeaders/AutoVersion.h"
#endif

namespace ResponsePlugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<TaskQueue> taskQueue,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::unique_ptr<IActionRunner> runner,
        std::shared_ptr<PluginCallback> callback) :
        m_taskQueue(std::move(taskQueue)),
        m_runner(std::move(runner)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
#ifdef SPL_BAZEL
        LOGINFO("Response Actions " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");
#endif
        LOGINFO("Completed initialization of Response Actions");

        while (true)
        {
            Task task;
            if (!m_taskQueue->pop(task, 60))
            {
                if (!m_runner->getIsRunning() && !m_actionQueue.empty())
                {
                    processAction(m_actionQueue[0].m_content, m_actionQueue[0].m_correlationId);
                    m_actionQueue.pop_front();
                }
            }
            else
            {
                switch (task.m_taskType)
                {
                    case Task::TaskType::STOP:
                        if (m_runner->getIsRunning())
                        {
                            m_runner->killAction();
                        }
                        return;
                    case Task::TaskType::CHECK_QUEUE:
                        if (!m_runner->getIsRunning() &&
                            !m_actionQueue.empty()) {
                            processAction(m_actionQueue[0].m_content,
                                          m_actionQueue[0].m_correlationId);
                            m_actionQueue.pop_front();
                        }
                        break;
                    case Task::TaskType::ACTION:
                        m_actionQueue.emplace_back(task);
                        if (!m_runner->getIsRunning()) {
                            processAction(m_actionQueue[0].m_content,
                                          m_actionQueue[0].m_correlationId);
                            m_actionQueue.pop_front();
                        }
                        break;
                }
            }
        }
    }

    void PluginAdapter::processAction(const std::string& action, const std::string& correlationId)
    {
        if (action.front() != '{')
        {
            LOGDEBUG("Ignoring action not in json format");
            return;
        }
        LOGDEBUG("Process action: " << action);
        auto [actionType, timeout] = PluginUtils::getType(action);
        if (timeout < 0)
        {
            LOGWARN("Action does not have a valid timeout set discarding it: " << action);
            return;
        }
        if (actionType.empty())
        {
            LOGWARN("Throwing away unknown action: " << action);
            return;
        }
        LOGINFO("Running action: " << correlationId);
        m_runner->runAction(action, correlationId, actionType, timeout);
    }

} // namespace ResponsePlugin