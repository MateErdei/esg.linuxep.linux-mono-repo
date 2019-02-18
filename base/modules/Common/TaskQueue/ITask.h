/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

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
    } // namespace TaskQueue
} // namespace Common
