/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include "../common/config.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include "Common/Logging/LoggingSetup.h"

#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/logger.h>

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

    // Log non-error messages to stdout
    log4cplus::SharedAppenderPtr stdout_appender(new log4cplus::ConsoleAppender(false));
    Common::Logging::LoggingSetup::applyPattern(stdout_appender, Common::Logging::LoggingSetup::GL_CONSOLE_PATTERN);
    log4cplus::Logger::getRoot().addAppender(stdout_appender);

    // Log error messages to stderr
    log4cplus::SharedAppenderPtr stderr_appender(new log4cplus::ConsoleAppender(true));
    stderr_appender->setThreshold(log4cplus::ERROR_LOG_LEVEL);
    Common::Logging::LoggingSetup::applyPattern(stderr_appender, Common::Logging::LoggingSetup::GL_CONSOLE_PATTERN);
    log4cplus::Logger::getRoot().addAppender(stderr_appender);

    if (!logfilepath.empty())
    {
        Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logfilepath);
    }
    Common::Logging::applyGeneralConfig(PLUGIN_NAME);
}

Logger::~Logger()
{
    log4cplus::Logger::shutdown();
}
