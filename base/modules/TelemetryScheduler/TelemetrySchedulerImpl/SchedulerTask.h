/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace TelemetrySchedulerImpl
{
    struct Task
    {
        enum class TaskType
        {
            ShutdownReceived
        };

        TaskType taskType;
    };
} // namespace TelemetrySchedulerImpl
