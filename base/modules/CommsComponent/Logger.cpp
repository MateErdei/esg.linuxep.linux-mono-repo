/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getCommsComponentLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("Comms");
    return STATIC_LOGGER;
}

log4cplus::Logger& getCommsComponentStartupLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("comms_startup");
    return STATIC_LOGGER;
}


void shutDownCommsComponentStartupLogger()
{
    getCommsComponentStartupLogger().shutdown();
}