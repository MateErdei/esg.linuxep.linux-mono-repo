/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>
log4cplus::Logger& getUtilityImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("UtilityImpl");
    return STATIC_LOGGER;
}