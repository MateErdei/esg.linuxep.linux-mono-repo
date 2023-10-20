// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getScanRequestQueueLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("ScanRequestQueue");
    return STATIC_LOGGER;
}