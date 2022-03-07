/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include "../common/config.h"
#include "common/StringUtils.h"

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

Logger::Logger(const std::string& fileName, log4cplus::LogLevel logLevel, bool isCommandLine)
{
    std::string logFilePath = fileName;
    if (!isCommandLine)
    {
        logFilePath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        logFilePath += "/plugins/av/log/";
        logFilePath += fileName;
        logFilePath += ".log";
    }

    log4cplus::initialize();

    // Log to stdout (Common::Logging::ConsoleLoggingSetup logs to stderr)
    log4cplus::SharedAppenderPtr stdout_appender(new log4cplus::ConsoleAppender(false));
    Common::Logging::LoggingSetup::applyPattern(stdout_appender, Common::Logging::LoggingSetup::GL_CONSOLE_PATTERN);
    log4cplus::Logger::getRoot().addAppender(stdout_appender);

    if (!logFilePath.empty())
    {
        setupFileLoggingWithPath(logFilePath);
    }

    if(logLevel != log4cplus::NOT_SET_LOG_LEVEL and isCommandLine)
    {
        applyCommandLineLevel(logLevel);
    }
    else
    {
        Common::Logging::applyGeneralConfig(PLUGIN_NAME);
    }
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

void Logger::applyCommandLineLevel(const log4cplus::LogLevel& CLSlogLevel)
{
    log4cplus::LogLevel helpMessageTempLevel{  CLSlogLevel };
    log4cplus::Logger::getRoot().setLogLevel(log4cplus::INFO_LOG_LEVEL);
    std::stringstream initMessage;
    initMessage << "Setting logger to log level: " << common::fromLogLevelToString(helpMessageTempLevel);
    log4cplus::Logger::getRoot().log(log4cplus::INFO_LOG_LEVEL, initMessage.str());

    log4cplus::Logger::getRoot().setLogLevel(CLSlogLevel);
    log4cplus::getLogLevelManager().pushToStringMethod(log4cplus::supportToStringMethod);
    log4cplus::getLogLevelManager().pushFromStringMethod(log4cplus::supportFromStringMethod);
}

