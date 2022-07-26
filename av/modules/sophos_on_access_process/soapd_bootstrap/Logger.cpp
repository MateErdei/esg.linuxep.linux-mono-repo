/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getSophosOnAccessBootstrapImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("soapd_bootstrap");
    return STATIC_LOGGER;
}
