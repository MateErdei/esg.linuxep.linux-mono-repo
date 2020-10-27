/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getProcLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("ProcUtils");
    return STATIC_LOGGER;
}
