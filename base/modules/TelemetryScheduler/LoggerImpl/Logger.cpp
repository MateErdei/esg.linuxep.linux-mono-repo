/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getTelemetrySchedulerLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("telemetryScheduler");
    return STATIC_LOGGER;
}
