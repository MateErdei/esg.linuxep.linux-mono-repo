/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getMountMonitorLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("MountMonitor");
    return STATIC_LOGGER;
}
