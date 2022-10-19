// Copyright 2018-2022, Sophos Limited.  All rights reserved.

#include "TaskQueue.h"

namespace Plugin
{
    void TaskQueue::push(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.emplace_back(std::move(task));
        m_cond.notify_one();
    }

    Task TaskQueue::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });
        Task val = std::move(m_list.front());
        m_list.pop_front();
        return val;
    }

    void TaskQueue::pushStop()
    {
        {
            std::lock_guard<std::mutex> lck(m_mutex);
            m_stopQueued = true;
        }

        Task stopTask{ .taskType = Task::TaskType::Stop, .Content = "" };
        push(std::move(stopTask));
    }

    bool TaskQueue::pop(Task& task, const std::chrono::time_point<std::chrono::steady_clock>& timeout_time)
    {
        std::unique_lock<std::mutex> lck(m_mutex);

        bool found = m_cond.wait_until(lck, timeout_time, [this](){ return !m_list.empty(); });

        if (found)
        {
            task = m_list.front();
            m_list.pop_front();
        }

        return found;
    }

    bool TaskQueue::empty()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        return m_list.empty();
    }

    bool TaskQueue::queueContainsPolicyTask()
    {
        std::unique_lock<std::mutex> lck(m_mutex);

        for (const auto &item : m_list)
        {
            if (item.taskType == Task::TaskType::Policy)
            {
                return true;
            }
        }

        return false;
    }

    bool TaskQueue::stoppableSleep(duration_t sleepTime)
    {
        std::unique_lock<std::mutex> lck(m_mutex);

        // Waits for period or for stop to be queued
        return m_cond.wait_for(lck, sleepTime, [this](){ return m_stopQueued; });
    }
} // namespace Plugin