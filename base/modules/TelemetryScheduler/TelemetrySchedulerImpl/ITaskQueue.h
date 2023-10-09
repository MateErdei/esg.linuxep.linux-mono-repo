/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SchedulerTask.h"

#include <string>

namespace TelemetrySchedulerImpl
{
    class ITaskQueue
    {
    public:
        virtual ~ITaskQueue() = default;

        virtual void push(SchedulerTask) = 0;
        virtual void pushPriority(SchedulerTask) = 0;
        virtual SchedulerTask pop() = 0;
        virtual bool pop(SchedulerTask& task, int timeout) = 0;
    };

} // namespace TelemetrySchedulerImpl
