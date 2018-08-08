/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LoggingSetup.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

namespace
{

    void setupLogging()
    {

        Path logDir = Common::ApplicationConfiguration::applicationPathManager().getBaseLogDirectory();
        Path logFilename = Common::FileSystem::join(logDir,"updatescheduler.log");

        log4cplus::initialize();

        GL_UPDSCH_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("updatescheduler"));

        log4cplus::tstring datePattern;
        const long maxFileSize = 10*1024*1024;
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

        // std::string pattern("%d{%m/%d/%y  %H:%M:%S}  - %m [%l]%n");
        const char* pattern = "%-7r [%d{%Y-%m-%dT%H:%M:%S.%Q}] %5p [%10.10t] %c <> %m%n";

        // Has to be an auto_ptr since the setLayout requires it.
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(pattern)); // NOLINT
        appender->setLayout(layout);

        GL_UPDSCH_LOGGER.addAppender(appender);

    }
}
UpdateScheduler::LoggingSetup::LoggingSetup()
{
    setupLogging();
}

UpdateScheduler::LoggingSetup::~LoggingSetup()
{
    log4cplus::Logger::shutdown();
}
