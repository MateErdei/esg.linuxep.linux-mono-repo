/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ConsoleLoggingSetup.h"

#include "LoggerConfig.h"
#include "LoggingSetup.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

using namespace Common::ApplicationConfigurationImpl;
using namespace Common::ApplicationConfiguration;

Common::Logging::ConsoleLoggingSetup::ConsoleLoggingSetup()
{
    consoleSetupLogging();
    applyGeneralConfig(Common::Logging::LOGFORTEST());
}

Common::Logging::ConsoleLoggingSetup::~ConsoleLoggingSetup()
{
    log4cplus::Logger::shutdown();
}

void Common::Logging::ConsoleLoggingSetup::consoleSetupLogging()
{
    log4cplus::initialize();

    log4cplus::SharedAppenderPtr appender(new log4cplus::ConsoleAppender(true));
    Common::Logging::LoggingSetup::applyPattern(appender, "%5p %m%n");

    log4cplus::Logger::getRoot().addAppender(appender);
}

Common::Logging::ConsoleLoggingSetup::ConsoleLoggingSetup(std::string log)
{
    consoleSetupLogging();
    applyGeneralConfig(log);
}
