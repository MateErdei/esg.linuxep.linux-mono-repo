/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/Scheduler.h>

#include <sys/stat.h>

static int telemetry_scheduler_main()
{
    umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
    return TelemetrySchedulerImpl::main_entry();
}

MAIN(telemetry_scheduler_main());
