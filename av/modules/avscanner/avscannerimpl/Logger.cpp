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

    log4cplus::initialize();

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

std::string fromLogLevelToString(log4cplus::LogLevel& logLevel)
{
    using sp = Common::Logging::SophosLogLevel;
    static std::vector<std::string> LogNames{ { "OFF" },     { "DEBUG" }, { "INFO" },
                                              { "SUPPORT" }, { "WARN" },  { "ERROR" } };

    static std::vector<sp> LogLevels{ sp::OFF, sp::DEBUG, sp::INFO, sp::SUPPORT, sp::WARN, sp::ERROR };

    auto ind_it = std::find(LogLevels.begin(), LogLevels.end(), logLevel);
    if (ind_it == LogLevels.end())
    {
        return std::string("Unknown (") + std::to_string(logLevel) + ")";
    }
    assert(std::distance(LogLevels.begin(), ind_it) >= 0);
    assert(std::distance(LogLevels.begin(), ind_it) < static_cast<int>(LogLevels.size()));
    return LogNames.at(std::distance(LogLevels.begin(), ind_it));
}

void Logger::applyCommandLineLevel(const Common::Logging::SophosLogLevel& CLSlogLevel)
{
    log4cplus::LogLevel helpMessageTempLevel{  CLSlogLevel };
    log4cplus::Logger::getRoot().setLogLevel(log4cplus::INFO_LOG_LEVEL);
    std::stringstream initMessage;
    initMessage << "Setting logger to log level: " << fromLogLevelToString(helpMessageTempLevel) << std::endl;
    log4cplus::Logger::getRoot().log(log4cplus::INFO_LOG_LEVEL, initMessage.str());

    log4cplus::Logger::getRoot().setLogLevel(CLSlogLevel);
    log4cplus::getLogLevelManager().pushToStringMethod(log4cplus::supportToStringMethod);
    log4cplus::getLogLevelManager().pushFromStringMethod(log4cplus::supportFromStringMethod);
}

