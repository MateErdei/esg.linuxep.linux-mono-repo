/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include "../common/config.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/LoggerConfig.h"
#include "Common/Logging/LoggingSetup.h"

#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
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

    // Log to stdout (Common::Logging::ConsoleLoggingSetup logs to stderr)
    log4cplus::SharedAppenderPtr stdout_appender(new log4cplus::ConsoleAppender(false));
    Common::Logging::LoggingSetup::applyPattern(stdout_appender, Common::Logging::LoggingSetup::GL_CONSOLE_PATTERN);
    log4cplus::Logger::getRoot().addAppender(stdout_appender);

    if (!logfilepath.empty())
    {
        setupFileLoggingWithPath(logfilepath);
    }
    Common::Logging::applyGeneralConfig(PLUGIN_NAME);
}

Logger::~Logger()
{
    log4cplus::Logger::shutdown();
}

// Same as Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(const std::string& logfilepath) but does not log to stderr
void Logger::setupFileLoggingWithPath(std::string logfilepath)
{
    log4cplus::tstring datePattern;
    const long maxFileSize = 10 * 1024 * 1024;
    const int maxBackupIndex = 10;
    const bool immediateFlush = false;
    const bool createDirs = true;
    log4cplus::SharedAppenderPtr appender(
            new log4cplus::RollingFileAppender(logfilepath, maxFileSize, maxBackupIndex, immediateFlush, createDirs));
    Common::Logging::LoggingSetup::applyDefaultPattern(appender);
    log4cplus::Logger::getRoot().addAppender(appender);
}