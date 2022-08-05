/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getOnAccessImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("OnAccessConfig");
    return STATIC_LOGGER;
}