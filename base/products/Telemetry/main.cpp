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

    std::shared_ptr<ICurlWrapper> curlWrapper = std::make_shared<CurlWrapper>();
    std::shared_ptr<IHttpSender> httpSender = std::make_shared<HttpSender>("https://t1.sophosupd.com/", curlWrapper);

    return Telemetry::main_entry(argc, argv, httpSender);
}

MAIN(telemetry_main(argc, argv))
