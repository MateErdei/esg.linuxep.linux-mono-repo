/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include "config.h"
#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getScanProcessMonitor()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("ScanProcessMonitor");
    return STATIC_LOGGER;
}
