/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getSophosOnAccessImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("SophosOnAccess");
    return STATIC_LOGGER;
}
