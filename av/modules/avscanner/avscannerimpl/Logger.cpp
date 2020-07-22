/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include "../common/config.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/ConsoleFileLoggingSetup.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"

#include <log4cplus/logger.h>

static bool GL_CONSOLE_LOGGING_SETUP = false;

log4cplus::Logger& getNamedScanRunnerLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("NamedScanRunner");
    return STATIC_LOGGER;
}

Logger::Logger(const std::string& fileName, bool isCommandLine)
{
    std::string logfilepath = fileName;
    if (!isCommandLine)
    {
        logfilepath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        logfilepath += "/plugins/av/log/";
        logfilepath += fileName;
        logfilepath += ".log";
    }

    if (GL_CONSOLE_LOGGING_SETUP)
    {
        Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logfilepath);
    }
    else
    {
        Common::Logging::ConsoleFileLoggingSetup::setupConsoleFileLoggingWithPath(logfilepath);
        GL_CONSOLE_LOGGING_SETUP = true;
    }
    Common::Logging::applyGeneralConfig(PLUGIN_NAME);
}

Logger::~Logger()
{
    log4cplus::Logger::shutdown();
}
