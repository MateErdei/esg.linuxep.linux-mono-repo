/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef COMMON_TASKQUEUEIMPL_TASKQUEUEIMPL_H
#define COMMON_TASKQUEUEIMPL_TASKQUEUEIMPL_H

#include <Common/TaskQueue/ITaskQueue.h>
#include <Common/TaskQueue/ITask.h>

#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace Common
{
    namespace TaskQueueImpl
    {
        using ITaskPtr = Common::TaskQueue::ITaskPtr;

        class TaskQueueImpl
            : public virtual Common::TaskQueue::ITaskQueue
        {
        public:
            void queueTask(ITaskPtr& task) override;
            ITaskPtr popTask() override;
        private:
            std::deque<ITaskPtr> m_tasks;
            std::mutex m_queueMutex;
            std::condition_variable m_condition;
        };
    }
}


#endif //COMMON_TASKQUEUEIMPL_TASKQUEUEIMPL_H
