/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getCommonLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("Common");
    return STATIC_LOGGER;
}
