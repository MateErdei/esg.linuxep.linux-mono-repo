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
            // If the schedule is in the past, this will run telemetry immediately, rather than scheduling a new time
            InitialWaitToRunTelemetry,
            WaitToRunTelemetry,
            RunTelemetry,
            CheckExecutableFinished,
            Policy
        };

        TaskType taskType;
        std::string content = "";
        std::string appId = "";

        bool operator==(SchedulerTask other) const
        {
            return (taskType == other.taskType && content == other.content && appId == other.appId);
        };
    };
} // namespace TelemetrySchedulerImpl
