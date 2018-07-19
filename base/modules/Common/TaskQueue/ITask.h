/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef COMMON_TASKQUEUE_ITASK_H
#define COMMON_TASKQUEUE_ITASK_H

namespace Common
{
    namespace TaskQueue
    {
        class ITask
        {
        public:
            virtual ~ITask() = default;
            virtual void run() = 0;
        };
    }
}

#endif //COMMON_TASKQUEUE_ITASK_H
