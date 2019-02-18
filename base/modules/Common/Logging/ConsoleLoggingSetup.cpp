/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ConsoleLoggingSetup.h"

#include <Common/Logging/LoggingSetup.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

Common::Logging::ConsoleLoggingSetup::ConsoleLoggingSetup()
{
    consoleSetupLogging();
}

Common::Logging::ConsoleLoggingSetup::~ConsoleLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::ConsoleLoggingSetup::consoleSetupLogging()
{
    log4cplus::initialize();

    log4cplus::SharedAppenderPtr appender(new log4cplus::ConsoleAppender(true));
    Common::Logging::LoggingSetup::applyDefaultPattern(appender);

    log4cplus::Logger::getRoot().addAppender(appender);
}
