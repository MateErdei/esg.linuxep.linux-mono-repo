/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TaskQueue.h"


namespace RemoteDiagnoseImpl
{
    void TaskQueue::push(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(task);
        m_cond.notify_one();
    }

    void TaskQueue::pushStop()
    {
        Task stopTask{ .taskType = Task::TaskType::STOP, .Content = "" };
        push(stopTask);
    }
    Task TaskQueue::pop()
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });
        Task task = m_list.front();
        m_list.pop_front();
        return task;
    }
    Task TaskQueue::pop(bool isRunning)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });

        if(!isRunning)
        {
            Task task = m_list.front();
            m_list.pop_front();
            return task;
        }
        else
        {
            Task task;
            std::list<Task>::iterator iter;
            for (iter = m_list.begin(); iter != m_list.end(); iter++)
            {
                if(iter->taskType != Task::TaskType::ACTION)
                {
                    task.taskType =iter->taskType;
                    task.Content =iter->Content;
                    break;
                }
            }
            m_list.erase(iter);
            return task;
        }

    }
} // namespace RemoteDiagnoseImpl
