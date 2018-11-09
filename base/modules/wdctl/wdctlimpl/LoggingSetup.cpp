/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LoggingSetup.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>


namespace
{

    void setupLogging()
    {

        Path logDir = Common::ApplicationConfiguration::applicationPathManager().getBaseLogDirectory();
        Path logFilename = Common::FileSystem::join(logDir,"wdctl.log");

        log4cplus::initialize();

        GL_WDCTL_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("wdctl"));

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

        std::unique_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(pattern)); // NOLINT
        appender->setLayout(std::move(layout));

        GL_WDCTL_LOGGER.addAppender(appender);

        appender = new log4cplus::ConsoleAppender(
                true, // logToStdErr
                true  // immediateFlush
                );
        std::unique_ptr<log4cplus::Layout> layout2(new log4cplus::PatternLayout("%-5p %m%n"));
        appender->setLayout(std::move(layout2));
        appender->setThreshold(log4cplus::WARN_LOG_LEVEL);
        GL_WDCTL_LOGGER.addAppender(appender);

    }
}
wdctl::wdctlimpl::LoggingSetup::LoggingSetup()
{
    setupLogging();
}

wdctl::wdctlimpl::LoggingSetup::~LoggingSetup()
{
    log4cplus::Logger::shutdown();
}
