/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TaskQueue.h"

namespace SchedulerImpl
{
    void TaskQueue::push(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(task);
        m_cond.notify_one();
    }

    Task TaskQueue::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });
        Task task = m_list.front();
        m_list.pop_front();
        return task;
    }
} // namespace TelemetrySchedulerImpl
