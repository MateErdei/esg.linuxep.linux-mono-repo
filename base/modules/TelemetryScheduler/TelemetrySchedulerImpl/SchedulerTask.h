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
        RunTelemetry
    };
} // namespace TelemetrySchedulerImpl
