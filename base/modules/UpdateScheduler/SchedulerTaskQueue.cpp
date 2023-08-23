/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerTaskQueue.h"

namespace UpdateScheduler
{
    void SchedulerTaskQueue::push(SchedulerTask task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(task);
        m_cond.notify_one();
    }

    bool SchedulerTaskQueue::pop(SchedulerTask& task, int timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        m_cond.wait_until(lock, now + std::chrono::seconds(timeout), [this] { return !m_list.empty(); });

        if (m_list.empty())
        {
            return false;
        }

        SchedulerTask val = m_list.front();
        m_list.pop_front();
        task = val;
        return true;
    }

    void SchedulerTaskQueue::pushStop()
    {
        SchedulerTask stopTask{ SchedulerTask::TaskType::Stop, "" };
        push(stopTask);
    }

    void SchedulerTaskQueue::pushFront(SchedulerTask task)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_list.push_front(task);
        m_cond.notify_one();
    }
} // namespace UpdateScheduler