// Copyright 2023 Sophos Limited. All rights reserved.

#include "TaskQueue.h"
namespace ResponsePlugin
{
    void TaskQueue::push(const Task& task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(task);
        m_cond.notify_one();
    }

    Task TaskQueue::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });
        Task val = m_list.front();
        m_list.pop_front();
        return val;
    }

    bool TaskQueue::pop(Task& task, int timeout)
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

    void TaskQueue::pushStop()
    {
        Task stopTask{ .m_taskType = Task::TaskType::STOP, .m_content = "" };
        push(stopTask);
    }

} // namespace ResponsePlugin