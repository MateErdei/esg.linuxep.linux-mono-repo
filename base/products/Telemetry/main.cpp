/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Telemetry/TelemetryImpl/Telemetry.h>

#include <Common/UtilityImpl/Main.h>

#include <sys/stat.h>

static int telemetry_main(int argc, char* argv[])
{
    umask(S_IRWXG | S_IRWXO); // Read and write for the owner
    return Telemetry::main_entry(argc, argv);
}

MAIN(telemetry_main(argc, argv))
