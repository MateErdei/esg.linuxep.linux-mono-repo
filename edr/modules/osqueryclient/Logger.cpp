/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getOSQueryClientLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("osqueryclient");
    return STATIC_LOGGER;
}
