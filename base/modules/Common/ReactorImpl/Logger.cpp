/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getReactorLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("reactor");
    return STATIC_LOGGER;
}
