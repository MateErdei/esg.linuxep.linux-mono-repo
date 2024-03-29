// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "TaskQueue.h"

namespace RemoteDiagnoseImpl
{
    void TaskQueue::push(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_back(task);
        m_cond.notify_one();
    }

    void TaskQueue::pushPriority(Task task)
    {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_list.push_front(task);
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

    Task TaskQueue::pop(bool isRunning)
    {
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [this] { return !m_list.empty(); });

        if (!isRunning)
        {
            Task task = m_list.front();
            m_list.pop_front();
            return task;
        }
        else
        {
            Task task;
            task.taskType = Task::TaskType::UNDEFINED;
            std::list<Task>::iterator iter;
            for (iter = m_list.begin(); iter != m_list.end(); ++iter)
            {
                if (iter->taskType != Task::TaskType::ACTION)
                {
                    task = *iter;
                    m_list.erase(iter);
                    break;
                }
            }
            return task;
        }
    }
} // namespace RemoteDiagnoseImpl
