/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/ConsoleFileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"

#include <log4cplus/logger.h>


log4cplus::Logger& getNamedScanRunnerLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("NamedScanRunner");
    return STATIC_LOGGER;
}

Logger::Logger(const std::string& scanName)
{
    std::string logfilepath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    logfilepath += "/plugins/av/log/";
    logfilepath += scanName;
    logfilepath += ".log";

    Common::Logging::ConsoleFileLoggingSetup::setupConsoleFileLoggingWithPath(logfilepath);
}

Logger::~Logger()
{
    log4cplus::Logger::shutdown();
}
