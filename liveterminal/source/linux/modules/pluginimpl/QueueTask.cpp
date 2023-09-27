/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

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

    Task QueueTask::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });
        Task val = m_list.front();
        m_list.pop_front();
        return val;
    }

    void QueueTask::pushStop()
    {
        Task stopTask{ .m_taskType = Task::TaskType::Stop, .m_content = "" };
        push(stopTask);
    }
} // namespace Plugin