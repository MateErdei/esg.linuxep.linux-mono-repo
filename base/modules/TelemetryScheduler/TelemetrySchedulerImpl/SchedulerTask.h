/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace TelemetrySchedulerImpl
{
    struct SchedulerTask
    {
        enum class TaskType
        {
            Shutdown,
            InitialWaitToRunTelemetry,
            WaitToRunTelemetry,
            RunTelemetry,
            CheckExecutableFinished,
            Policy
        };

        TaskType taskType;
        std::string content;
        std::string appId;
    };
} // namespace TelemetrySchedulerImpl
