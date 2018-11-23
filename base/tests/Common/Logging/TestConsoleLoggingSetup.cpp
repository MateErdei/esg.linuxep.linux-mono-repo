/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "TestConsoleLoggingSetup.h"

#include <Common/Logging/LoggingSetup.h>

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/loggingmacros.h>

Test::TestConsoleLoggingSetup::TestConsoleLoggingSetup()
{
    consoleSetupLogging();
}

Test::TestConsoleLoggingSetup::~TestConsoleLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Test::TestConsoleLoggingSetup::consoleSetupLogging()
{

    log4cplus::initialize();

    log4cplus::SharedAppenderPtr appender(
            new log4cplus::ConsoleAppender(true)
    );
    Common::Logging::LoggingSetup::applyPattern(appender,"%m%n");

    log4cplus::Logger::getRoot().addAppender(appender);
}
