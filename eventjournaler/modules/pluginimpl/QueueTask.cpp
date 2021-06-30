/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "QueueTask.h"
namespace Plugin
{
    void QueueTask::push(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(task);
        m_cond.notify_one();
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
        Task stopTask{ .taskType = Task::TaskType::Stop, .Content = "" };
        push(stopTask);
    }
} // namespace Plugin