/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ConsoleFileLoggingSetup.h"

#include "LoggerConfig.h"
#include "LoggingSetup.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/LoggingSetup.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

using namespace Common::ApplicationConfigurationImpl;
using namespace Common::ApplicationConfiguration;


Common::Logging::ConsoleFileLoggingSetup::ConsoleFileLoggingSetup(const std::string& logbase, bool lowpriv)
{
    setupConsoleFileLogging(logbase, lowpriv);
    applyGeneralConfig(logbase);
}

Common::Logging::ConsoleFileLoggingSetup::~ConsoleFileLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::ConsoleFileLoggingSetup::setupConsoleFileLogging(const std::string& logbase, bool lowpriv)
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
    setupConsoleFileLoggingWithPath(logFilename);
}

void Common::Logging::ConsoleFileLoggingSetup::setupConsoleFileLoggingWithPath(const std::string& logfilepath)
{
    log4cplus::initialize();

    log4cplus::tstring datePattern;
    const long maxFileSize = 10 * 1024 * 1024;
    const int maxBackupIndex = 10;
    const bool immediateFlush = true;
    const bool createDirs = true;

    log4cplus::SharedAppenderPtr fileAppender(
            new log4cplus::RollingFileAppender(logfilepath, maxFileSize, maxBackupIndex, immediateFlush, createDirs));
    Common::Logging::LoggingSetup::applyDefaultPattern(fileAppender);
    log4cplus::Logger::getRoot().addAppender(fileAppender);

    log4cplus::SharedAppenderPtr consoleAppender(new log4cplus::ConsoleAppender(true));
    Common::Logging::LoggingSetup::applyPattern(consoleAppender, "%m%n");

    log4cplus::Logger::getRoot().addAppender(consoleAppender);
}
