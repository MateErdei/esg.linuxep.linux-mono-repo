/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "FileLoggingSetup.h"
#include "LoggerConfig.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/LoggingSetup.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>


Common::Logging::FileLoggingSetup::FileLoggingSetup(const std::string& logbase, bool lowpriv)
{
    setupFileLogging(logbase, lowpriv);
    applyGeneralConfig(logbase);

}

Common::Logging::FileLoggingSetup::~FileLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::FileLoggingSetup::setupFileLogging(const std::string& logbase, bool lowpriv)
{
    Path logDir;

    std::string logfilename = Common::UtilityImpl::StringUtils::endswith(logbase, ".log") ? logbase : logbase + ".log";
    if (lowpriv)
    {
        logDir = Common::ApplicationConfiguration::applicationPathManager().getBaseSophossplLogDirectory();
    }
    else
    {
        logDir = Common::ApplicationConfiguration::applicationPathManager().getBaseLogDirectory();
    }

    Path logFilename = Common::FileSystem::join(logDir, logfilename);
    setupFileLoggingWithPath(logFilename);
}

void Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(const std::string& logfilepath)
{
    log4cplus::initialize();

    log4cplus::tstring datePattern;
    const long maxFileSize = 10 * 1024 * 1024;
    const int maxBackupIndex = 10;
    const bool immediateFlush = true;
    const bool createDirs = true;
    log4cplus::SharedAppenderPtr appender(
        new log4cplus::RollingFileAppender(logfilepath, maxFileSize, maxBackupIndex, immediateFlush, createDirs));
    Common::Logging::LoggingSetup::applyDefaultPattern(appender);
    log4cplus::Logger::getRoot().addAppender(appender);

    // Log error messages to stderr
    log4cplus::SharedAppenderPtr stderr_appender(new log4cplus::ConsoleAppender(true));
    stderr_appender->setThreshold(log4cplus::ERROR_LOG_LEVEL);
    Common::Logging::LoggingSetup::applyPattern(stderr_appender, Common::Logging::LoggingSetup::GL_CONSOLE_PATTERN);
    log4cplus::Logger::getRoot().addAppender(stderr_appender);
}
