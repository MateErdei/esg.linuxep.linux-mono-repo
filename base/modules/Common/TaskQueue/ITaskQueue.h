/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ITask.h"

#include <memory>

namespace Common
{
    namespace TaskQueue
    {
        using ITaskPtr = std::unique_ptr<Common::TaskQueue::ITask>;

        class ITaskQueue
        {
        public:
            virtual ~ITaskQueue() = default;
            virtual void queueTask(ITaskPtr task) = 0;
            virtual ITaskPtr popTask() = 0;
        };

        using ITaskQueueSharedPtr = std::shared_ptr<Common::TaskQueue::ITaskQueue>;
    } // namespace TaskQueue
} // namespace Common
