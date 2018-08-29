/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "FileLoggingSetup.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>
#include <Common/Logging/LoggingSetup.h>
#include <log4cplus/loggingmacros.h>

Common::Logging::FileLoggingSetup::FileLoggingSetup(const std::string& logbase)
{
    setupFileLogging(logbase);

}

Common::Logging::FileLoggingSetup::~FileLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::FileLoggingSetup::setupFileLogging(const std::string& logbase)
{
    Path logDir = Common::ApplicationConfiguration::applicationPathManager().getBaseLogDirectory();
    Path logFilename = Common::FileSystem::join(logDir, logbase+".log");

    log4cplus::initialize();

    log4cplus::tstring datePattern;
    const long maxFileSize = 10 * 1024 * 1024;
    const int maxBackupIndex = 10;
    const bool immediateFlush = true;
    const bool createDirs = true;
    log4cplus::SharedAppenderPtr appender(
            new log4cplus::RollingFileAppender(
                    logFilename,
                    maxFileSize,
                    maxBackupIndex,
                    immediateFlush,
                    createDirs
            )
    );
    Common::Logging::LoggingSetup::applyDefaultPattern(appender);
    log4cplus::Logger::getRoot().addAppender(appender);


    // Log error messages to stderr
    log4cplus::SharedAppenderPtr stderr_appender(
            new log4cplus::ConsoleAppender(true)
    );
    stderr_appender->setThreshold(log4cplus::ERROR_LOG_LEVEL);
    Common::Logging::LoggingSetup::applyPattern(stderr_appender, Common::Logging::LoggingSetup::GL_CONSOLE_PATTERN);
    log4cplus::Logger::getRoot().addAppender(stderr_appender);

    // Log everything to stdout - TODO remove this once product tests updated
    log4cplus::SharedAppenderPtr stdout_appender(
            new log4cplus::ConsoleAppender(false)
    );
    Common::Logging::LoggingSetup::applyPattern(stdout_appender, "%m%n");
    log4cplus::Logger::getRoot().addAppender(stdout_appender);
}
