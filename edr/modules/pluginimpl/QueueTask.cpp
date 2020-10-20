/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "QueueTask.h"
namespace Plugin
{
    void QueueTask::push(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.emplace_back(std::move(task));
        m_cond.notify_one();
    }

    bool QueueTask::pop(Task& task, int timeout)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        m_cond.wait_until(lck, now + std::chrono::seconds(timeout),[this] { return !m_list.empty(); });

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
        Task stopTask{ .m_taskType = Task::TaskType::OSQUERYPROCESSFAILEDTOSTART, .m_content = "" };
        push(stopTask);
    }

    void QueueTask::pushOsqueryProcessFinished()
    {
        Task stopTask{ .m_taskType = Task::TaskType::OSQUERYPROCESSFINISHED, .m_content = "" };
        push(stopTask);
    }

    void QueueTask::pushRestartOsquery()
    {
        Task stopTask{ .m_taskType = Task::TaskType::RESTARTOSQUERY, .m_content = "" };
        push(stopTask);
    }

    void QueueTask::pushPolicy(const std::string& appId, const std::string& policyXMl)
    {
        Task task{ .m_taskType = Task::TaskType::Policy, .m_content = policyXMl, .m_correlationId = "", .m_appId = appId };
        push(task);
    }
} // namespace Plugin