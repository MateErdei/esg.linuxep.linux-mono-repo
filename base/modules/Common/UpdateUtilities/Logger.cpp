// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getUpdateUtilitiesLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("UpdateUtilities");
    return STATIC_LOGGER;
}
