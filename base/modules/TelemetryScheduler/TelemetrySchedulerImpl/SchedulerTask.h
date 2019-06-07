/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace TelemetrySchedulerImpl
{
    enum class SchedulerTask
    {
        Shutdown,
        InitialWaitToRunTelemetry,
        WaitToRunTelemetry,
        RunTelemetry,
        CheckExecutableFinished
    };
} // namespace TelemetrySchedulerImpl
