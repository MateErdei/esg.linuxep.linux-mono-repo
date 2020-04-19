/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getTelemetryHelperLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("TelemetryHelperImpl");
    return STATIC_LOGGER;
}
