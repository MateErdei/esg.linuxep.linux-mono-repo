// Copyright 2023 Sophos Limited. All rights reserved.

#include "Telemetry.h"

#include "Common/Main/Main.h"
#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/OSUtilitiesImpl/PlatformUtils.h"

#include <log4cplus/logger.h>

#include <fstream>
#include <sstream>

static void setupLogging()
{
    bool debugMode = static_cast<bool>(getenv("DEBUG_THIN_INSTALLER"));
    auto root = log4cplus::Logger::getRoot();
    if (debugMode)
    {
        root.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
    }
    else
    {
        root.setLogLevel(log4cplus::WARN_LOG_LEVEL);
    }
}

/**
 * argv[1] = ${SOPHOS_TEMP_DIRECTORY}/credentials.txt
 * @param argc
 * @param argv
 * @return
 */
static int inner_main(int argc, char* argv[])
{
    Common::Logging::ConsoleLoggingSetup logging;
    setupLogging();
    std::vector<std::string> args;
    for (auto i=1; i<argc; ++i)
    {
        args.emplace_back(argv[i]);
    }
    auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
    auto httpRequester = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
    thininstaller::telemetry::Telemetry telemetry{args, httpRequester};
    auto* fs = Common::FileSystem::fileSystem();
    auto platform = std::make_shared<Common::OSUtilitiesImpl::PlatformUtils>();
    return telemetry.run(fs, *platform);
}

MAIN(inner_main(argc, argv))
