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

    SchedulerTask SchedulerTaskQueue::pop() {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this]{return !m_list.empty();});
        SchedulerTask val = m_list.front();
        m_list.pop_front();
        return val;
    }

    void SchedulerTaskQueue::pushStop()
    {
        SchedulerTask stopTask{SchedulerTask::TaskType::Stop};
        push(stopTask);
    }
}