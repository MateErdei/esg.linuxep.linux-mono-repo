/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <TelemetryScheduler/TelemetrySchedulerImpl/TelemetryScheduler.h>

#include <Common/UtilityImpl/Main.h>

static int telemetry_scheduler_main()
{
    return TelemetryScheduler::main_entry();
}

MAIN(telemetry_scheduler_main());
