/******************************************************************************************************

Copyright 2018-2019 Sophos Limited.  All rights reserved.

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
        Task stopTask{ .taskType = Task::TaskType::STOP, .Content = "" };
        push(stopTask);
    }

    void QueueTask::pushOsqueryProcessDelayRestart()
    {
        Task stopTask{ .taskType = Task::TaskType::OSQUERYPROCESSFAILEDTOSTART, .Content = "" };
        push(stopTask);
    }

    void QueueTask::pushOsqueryProcessFinished()
    {
        Task stopTask{ .taskType = Task::TaskType::OSQUERYPROCESSFINISHED, .Content = "" };
        push(stopTask);
    }

    void QueueTask::pushRestartOsquery()
    {
        Task stopTask{ .taskType = Task::TaskType::RESTARTOSQUERY, .Content = "" };
        push(stopTask);
    }
} // namespace Plugin