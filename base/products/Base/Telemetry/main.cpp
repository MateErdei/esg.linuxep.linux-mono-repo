// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Common/UtilityImpl/Main.h"
#include "Telemetry/TelemetryImpl/Telemetry.h"
#include <sys/stat.h>

static int telemetry_main(int argc, char* argv[])
{
    umask(S_IRWXG | S_IRWXO); // Read and write for the owner
    return Telemetry::main_entry(argc, argv);
}

MAIN(telemetry_main(argc, argv))
