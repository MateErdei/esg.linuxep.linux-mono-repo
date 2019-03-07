/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getWatchdogImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("watchdog");
    return STATIC_LOGGER;
}