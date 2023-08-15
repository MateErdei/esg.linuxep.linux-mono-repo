// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getTelemetryLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("telemetry");
    return STATIC_LOGGER;
}
