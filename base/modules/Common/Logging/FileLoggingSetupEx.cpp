/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "FileLoggingSetupEx.h"

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

Common::Logging::FileLoggingSetupEx::FileLoggingSetupEx(const std::string& logbase, bool lowpriv)
{
    setupFileLogging(logbase, lowpriv);
    applyGeneralConfig(logbase);
}

Common::Logging::FileLoggingSetupEx::FileLoggingSetupEx(const Path& logdir, const std::string& logbase)
{
    Path logPath = Common::FileSystem::join(logdir, logbase + ".log");
    setupFileLoggingWithPath(logPath, "");
    applyGeneralConfig(logbase);
}

Common::Logging::FileLoggingSetupEx::~FileLoggingSetupEx()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::FileLoggingSetupEx::setupFileLogging(const std::string& logbase, bool lowpriv)
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
    setupFileLoggingWithPath(logFilename, "");
}

void Common::Logging::FileLoggingSetupEx::setupFileLoggingWithPath(const std::string& logfilepath, const std::string& instanceName)
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
    auto logger = Common::Logging::getInstance(instanceName);
    logger.addAppender(appender);
    logger.setAdditivity(false);
}