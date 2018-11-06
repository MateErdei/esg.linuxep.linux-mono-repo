/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ConsoleLoggingSetup.h"
#include "LoggingSetup.h"
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/loggingmacros.h>

LoggingAPI::ConsoleLoggingSetup::ConsoleLoggingSetup()
{
    consoleSetupLogging();
}

LoggingAPI::ConsoleLoggingSetup::~ConsoleLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void LoggingAPI::ConsoleLoggingSetup::consoleSetupLogging()
{
    log4cplus::initialize();

    log4cplus::SharedAppenderPtr appender(
            new log4cplus::ConsoleAppender(true)
    );
    LoggingAPI::LoggingSetup::applyDefaultPattern(appender);

    log4cplus::Logger::getRoot().addAppender(appender);
}
