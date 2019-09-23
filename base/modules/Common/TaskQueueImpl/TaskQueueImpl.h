/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/TaskQueue/ITask.h>
#include <Common/TaskQueue/ITaskQueue.h>

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

namespace Common
{
    namespace TaskQueueImpl
    {
        using ITaskPtr = Common::TaskQueue::ITaskPtr;

        class TaskQueueImpl : public virtual Common::TaskQueue::ITaskQueue
        {
        public:
            void queueTask(ITaskPtr task) override;
            ITaskPtr popTask() override;

        protected:
            std::deque<ITaskPtr> m_tasks;
            std::mutex m_queueMutex;
            std::condition_variable m_condition;
        };
    } // namespace TaskQueueImpl
} // namespace Common
