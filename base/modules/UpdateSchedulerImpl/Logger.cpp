/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getUpdateSchedulerImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("updatescheduler");
    return STATIC_LOGGER;
}