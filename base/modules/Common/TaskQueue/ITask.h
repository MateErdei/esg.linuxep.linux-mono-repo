/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace Common::TaskQueue
{
    class ITask
    {
    public:
        virtual ~ITask() = default;
        virtual void run() = 0;
    };
} // namespace Common::TaskQueue
