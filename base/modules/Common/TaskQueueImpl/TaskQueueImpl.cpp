/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TaskQueueImpl.h"

using ITaskPtr = Common::TaskQueue::ITaskPtr;

void Common::TaskQueueImpl::TaskQueueImpl::queueTask(ITaskPtr task)
{
    if (task == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_tasks.push_back(std::move(task));

    m_condition.notify_all();
}

Common::TaskQueueImpl::ITaskPtr Common::TaskQueueImpl::TaskQueueImpl::popTask()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);

    m_condition.wait(lock, [this] { return !m_tasks.empty(); });

    auto task = std::move(m_tasks.front());
    m_tasks.pop_front();

    return task;
}
