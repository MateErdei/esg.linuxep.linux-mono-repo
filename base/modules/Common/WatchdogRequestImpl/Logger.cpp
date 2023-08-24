// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getWatchdogRequestImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("WatchdogRequestImpl");
    return STATIC_LOGGER;
}
