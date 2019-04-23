/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/FileLoggingSetup.h>
#include <Telemetry/TelemetryImpl/Telemetry.h>

#include <Common/UtilityImpl/Main.h>

static int telemetry_main(int argc, char* argv[])
{
    // Configure logging
    Common::Logging::FileLoggingSetup loggerSetup("telemetry", true);

    return Telemetry::main_entry(argc, argv);
}

MAIN(telemetry_main(argc, argv))
