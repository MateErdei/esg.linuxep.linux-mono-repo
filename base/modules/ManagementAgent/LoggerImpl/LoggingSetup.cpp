/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LoggingSetup.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

log4cplus::Logger GL_MANAGEMENTAGENT_LOGGER;// = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("managementagent")); //NOLINT

namespace
{

    void setupLogging()
    {

        Path logDir = Common::ApplicationConfiguration::applicationPathManager().getBaseLogDirectory();
        Path logFilename = Common::FileSystem::join(logDir, "sophos_managementagent.log");

        log4cplus::initialize();

        GL_MANAGEMENTAGENT_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("managementagent")); //NOLINT

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

        // std::string pattern("%d{%m/%d/%y  %H:%M:%S}  - %m [%l]%n");
        const char* pattern = "%-7r [%d{%Y-%m-%dT%H:%M:%S.%Q}] %5p [%10.10t] %c <> %m%n";

        // Has to be an auto_ptr since the setLayout requires it.
        std::auto_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(pattern)); // NOLINT
        appender->setLayout(layout);

        GL_MANAGEMENTAGENT_LOGGER.addAppender(appender);

    }

    void consoleSetupLogging()
    {

        log4cplus::initialize();
        GL_MANAGEMENTAGENT_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("managementagent")); //NOLINT

        log4cplus::SharedAppenderPtr appender(
                new log4cplus::ConsoleAppender()
        );
        GL_MANAGEMENTAGENT_LOGGER.addAppender(appender);
    }
}

namespace ManagementAgent
{
    namespace LoggerImpl
    {
        LoggingSetup::LoggingSetup()
        {
            setupLogging();
        }

        LoggingSetup::LoggingSetup(int)
        {
            consoleSetupLogging();
        }


        LoggingSetup::~LoggingSetup()
        {
            log4cplus::Logger::shutdown();
        }

    }
}
