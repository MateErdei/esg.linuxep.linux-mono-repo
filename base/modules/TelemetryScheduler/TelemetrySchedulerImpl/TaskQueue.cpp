// Copyright 2019-2023 Sophos Limited. All rights reserved.
#include "TaskQueue.h"

#include "SchedulerTask.h"

namespace TelemetrySchedulerImpl
{
    void TaskQueue::push(SchedulerTask task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(task);
        m_cond.notify_one();
    }

    void TaskQueue::pushPriority(SchedulerTask task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_front(task);
        m_cond.notify_one();
    }

    SchedulerTask TaskQueue::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });
        SchedulerTask task = m_list.front();
        m_list.pop_front();
        return task;
    }

    bool TaskQueue::pop(SchedulerTask& task, int timeout)
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
} // namespace TelemetrySchedulerImpl
