/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

namespace plugin::manager::scanprocessmonitor
{
    log4cplus::Logger& getScanProcessMonitorLogger()
    {
        static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("ScanProcessMonitor");
        return STATIC_LOGGER;
    }
}
