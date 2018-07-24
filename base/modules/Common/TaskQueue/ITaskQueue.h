/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef COMMON_TASKQUEUE_ITASKQUEUE_H
#define COMMON_TASKQUEUE_ITASKQUEUE_H

#include <memory>
#include "ITask.h"

namespace Common
{
    namespace TaskQueue
    {
        using ITaskPtr = std::unique_ptr<Common::TaskQueue::ITask>;

        class ITaskQueue
        {
        public:
            virtual ~ITaskQueue() = default;
            virtual void queueTask(ITaskPtr& task) = 0;
            virtual ITaskPtr popTask() = 0;
        };

        using ITaskQueueSharedPtr = std::shared_ptr<Common::TaskQueue::ITaskQueue>;
    }
}

#endif //COMMON_TASKQUEUE_ITASKQUEUE_H
