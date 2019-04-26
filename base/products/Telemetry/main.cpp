/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Telemetry/TelemetryImpl/Telemetry.h>

#include <Common/UtilityImpl/Main.h>

static int telemetry_main(int argc, char* argv[])
{
    return Telemetry::main_entry(argc, argv);
}

MAIN(telemetry_main(argc, argv))
