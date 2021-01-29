/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getThreatScannerLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("ThreatScanner");
    return STATIC_LOGGER;
}

log4cplus::Logger& getSusiDebugLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("SUSI_DEBUG");
    return STATIC_LOGGER;
}