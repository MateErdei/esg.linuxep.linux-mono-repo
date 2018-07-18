/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "TaskQueue.h"

#include <iostream>
#include <zconf.h>

namespace ManagementAgent
{
    namespace McsRouterPluginCommunicationImpl
    {
        TaskQueue::TaskQueue()
        : m_queueMutex()
        , m_condition()
        {
            m_queueMutex = std::unique_ptr<std::mutex>(new std::mutex);
            m_condition = std::unique_ptr<std::condition_variable>(new std::condition_variable);
            process = false;
        }

        TaskQueue::~TaskQueue()
        {
        }
        void TaskQueue::push(Task task)
        {
            std::lock_guard<std::mutex> lock(*m_queueMutex);
            m_queueMutex->unlock();
            m_tasks.push_back(task);

            m_condition->notify_all();
        }

        Task TaskQueue::pop()
        {

            std::unique_lock<std::mutex> lock(*m_queueMutex);

            m_condition->wait(lock, [this]{return !m_tasks.empty();});

            Task task = m_tasks.front();
            m_tasks.pop_front();

            return task;
        }
    }

}