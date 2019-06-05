/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace TelemetrySchedulerImpl
{
    enum class Task
    {
        Shutdown,
        WaitToRunTelemetry,
        RunTelemetry,
        CheckExecutableFinished
    };

    class ITaskQueue
    {
    public:
        virtual void push(Task) = 0;
        virtual void pushPriority(Task) = 0;
        virtual Task pop() = 0;
    };

} // namespace TelemetrySchedulerImpl

