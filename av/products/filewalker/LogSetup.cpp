/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <log4cplus/logger.h>

LogSetup::LogSetup()
{
    Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
}

LogSetup::~LogSetup()
{
    log4cplus::Logger::shutdown();
}
