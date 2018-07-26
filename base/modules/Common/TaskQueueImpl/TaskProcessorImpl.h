/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once



#include <Common/TaskQueue/ITaskProcessor.h>
#include <Common/TaskQueue/ITaskQueue.h>
#include <Common/Threads/AbstractThread.h>

namespace Common
{
    namespace TaskQueueImpl
    {
        using ITaskQueueSharedPtr = std::shared_ptr<Common::TaskQueue::ITaskQueue>;

        class TaskProcessorImplThread
            : public Common::Threads::AbstractThread
        {
        public:
            explicit TaskProcessorImplThread(ITaskQueueSharedPtr taskQueue);
            std::shared_ptr<Common::TaskQueue::ITaskQueue> m_taskQueue;
        private:
            void run() override;
        };



        class TaskProcessorImpl : public virtual Common::TaskQueue::ITaskProcessor
        {
        public:
            explicit TaskProcessorImpl(ITaskQueueSharedPtr taskQueue);


            void start() override;

            void stop() override;

        private:
            TaskProcessorImplThread m_thread;
        };
    }
}



