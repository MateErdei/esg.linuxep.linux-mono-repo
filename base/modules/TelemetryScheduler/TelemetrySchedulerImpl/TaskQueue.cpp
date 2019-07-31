/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
} // namespace TelemetrySchedulerImpl
