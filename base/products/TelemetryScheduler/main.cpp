/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/Scheduler.h>

static int telemetry_scheduler_main()
{
    return SchedulerImpl::main_entry();
}

MAIN(telemetry_scheduler_main());
