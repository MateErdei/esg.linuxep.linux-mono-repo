/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "FileLoggingSetup.h"
#include "LoggingSetup.h"
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/loggingmacros.h>

LoggingAPI::FileLoggingSetup::FileLoggingSetup(const std::string& logbase)
{
    setupFileLogging(logbase);

}

LoggingAPI::FileLoggingSetup::~FileLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void LoggingAPI::FileLoggingSetup::setupFileLogging(const std::string& logFilePath)
{
    log4cplus::initialize();

    log4cplus::tstring datePattern;
    const long maxFileSize = 10 * 1024 * 1024;
    const int maxBackupIndex = 10;
    const bool immediateFlush = true;
    const bool createDirs = true;
    log4cplus::SharedAppenderPtr appender(
            new log4cplus::RollingFileAppender(
                    logFilePath,
                    maxFileSize,
                    maxBackupIndex,
                    immediateFlush,
                    createDirs
            )
    );
    LoggingAPI::LoggingSetup::applyDefaultPattern(appender);
    log4cplus::Logger::getRoot().addAppender(appender);


    // Log error messages to stderr
    log4cplus::SharedAppenderPtr stderr_appender(
            new log4cplus::ConsoleAppender(true)
    );
    stderr_appender->setThreshold(log4cplus::ERROR_LOG_LEVEL);
    LoggingAPI::LoggingSetup::applyPattern(stderr_appender, LoggingAPI::LoggingSetup::GL_CONSOLE_PATTERN);
    log4cplus::Logger::getRoot().addAppender(stderr_appender);

}
