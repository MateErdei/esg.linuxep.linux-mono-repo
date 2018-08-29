/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LoggingSetup.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

#include <log4cplus/logger.h>

namespace ManagementAgent
{
    namespace LoggerImpl
    {
        LoggingSetup::LoggingSetup()
        {
            Common::Logging::FileLoggingSetup::setupFileLogging("sophos_managementagent");
        }

        LoggingSetup::LoggingSetup(int)
        {
            Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
        }


        LoggingSetup::~LoggingSetup()
        {
            log4cplus::Logger::shutdown();
        }

    }
}

log4cplus::Logger& getManagementAgentLogger()
{
    static log4cplus::Logger STATIC_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("managementagent"));
    return STATIC_LOGGER;
}
