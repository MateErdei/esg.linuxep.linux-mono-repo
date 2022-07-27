/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/FileLoggingSetup.h"

#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>

#include <iterator>


log4cplus::Logger& getSophosOnAccessLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("SophosOnAccess");
    return STATIC_LOGGER;
}

namespace
{
    void setupFileLoggingWithPath(const std::string& logdirectory, const std::string& logbase)
    {
        auto logfilepath = logdirectory + "/on_access.log";

        log4cplus::initialize();

        log4cplus::tstring datePattern;
        constexpr long maxFileSize = 1 * 1024 * 1024;
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


        Common::Logging::applyGeneralConfig(logbase); // Sets the threshold for the root logger

    }
}

LogSetup::LogSetup()
{
    auto sophosInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    auto pluginInstall = sophosInstall + "/plugins/av";

    auto logdirectory = pluginInstall + "/log";
    std::cout<< "Installed here: "<< logdirectory << std::endl;
    setupFileLoggingWithPath(logdirectory, "av");
}

LogSetup::~LogSetup()
{
    log4cplus::Logger::shutdown();
}