/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef COMMON_TASKQUEUE_ITASKPROCESSOR_H
#define COMMON_TASKQUEUE_ITASKPROCESSOR_H

namespace Common
{
    namespace TaskQueue
    {
        class ITaskProcessor
        {
        public:
            virtual ~ITaskProcessor() = default;

            /**
             * Start the task processor.
             * Will wait for the processor thread to be started before returning.
             */
            virtual void start() = 0;

            /**
             * Stops the task processor thread.
             * Will wait for all tasks in the queue to be finished, and the thread to exit before returning.
             */
            virtual void stop() = 0;
        };
    }
}
#endif //COMMON_TASKQUEUE_ITASKPROCESSOR_H
