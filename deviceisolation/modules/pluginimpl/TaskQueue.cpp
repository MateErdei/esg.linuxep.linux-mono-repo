// Copyright 2023 Sophos Limited. All rights reserved.

#include "TaskQueue.h"

namespace Plugin
{
    void TaskQueue::push(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(std::move(task));
        m_cond.notify_one();
    }

    bool TaskQueue::pop(Task& task, int timeout)
    {
        return pop(task, std::chrono::seconds(timeout));
    }

    bool TaskQueue::pop(Task& task, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_cond.wait_for(lock, timeout,[this] { return !m_list.empty(); });

        if (m_list.empty())
        {
            return false;
        }

        Task val = m_list.front();
        m_list.pop_front();
        task =  val;
        return true;
    }

    void TaskQueue::pushStop()
    {
        Task stopTask{ .taskType = Task::TaskType::Stop, .Content = "" };
        push(stopTask);
    }
} // namespace Plugin