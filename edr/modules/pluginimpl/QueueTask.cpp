/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "QueueTask.h"

#include <algorithm>
namespace Plugin
{
    void QueueTask::push(const Task& task)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_list.emplace_back(std::move(task));
        m_cond.notify_one();
    }

    void QueueTask::pushIfNotAlreadyInQueue(Task task)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto result = std::find_if(
            m_list.begin(),
            m_list.end(),
            [task](const Task& taskInList){return (taskInList.m_taskType == task.m_taskType);});

        if (result == m_list.end())
        {
            m_list.emplace_back(std::move(task));
            m_cond.notify_one();
        }
    }

    bool QueueTask::pop(Task& task, int timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        m_cond.wait_until(lock, now + std::chrono::seconds(timeout),[this] { return !m_list.empty(); });

        if (m_list.empty())
        {
            return false;
        }

        Task val = m_list.front();
        m_list.pop_front();
        task =  val;
        return true;
    }

    void QueueTask::pushStop()
    {
        Task stopTask{ .m_taskType = Task::TaskType::STOP, .m_content = "" };
        push(stopTask);
    }

    void QueueTask::pushOsqueryProcessDelayRestart()
    {
        Task stopTask{ .m_taskType = Task::TaskType::OSQUERY_PROCESS_FAILED_TO_START, .m_content = "" };
        push(stopTask);
    }

    void QueueTask::pushOsqueryProcessFinished()
    {
        Task stopTask{ .m_taskType = Task::TaskType::OSQUERY_PROCESS_FINISHED, .m_content = "" };
        push(stopTask);
    }

    void QueueTask::pushStartOsquery()
    {
        Task stopTask{ .m_taskType = Task::TaskType::START_OSQUERY, .m_content = "" };
        push(stopTask);
    }

    void QueueTask::pushPolicy(const std::string& appId, const std::string& policyXMl)
    {
        Task task{ .m_taskType = Task::TaskType::POLICY, .m_content = policyXMl, .m_correlationId = "", .m_appId = appId };
        push(task);
    }

    void QueueTask::pushOsqueryRestart(const std::string& reason)
    {
        Task restartTask{ .m_taskType = Task::TaskType::QUEUE_OSQUERY_RESTART, .m_content = reason };
        pushIfNotAlreadyInQueue(restartTask);
    }

    void QueueTask::clearQueue()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_list.clear();
    }
} // namespace Plugin