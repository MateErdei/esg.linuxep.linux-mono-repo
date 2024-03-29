// Copyright 2020-2023 Sophos Limited. All rights reserved.
#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getLoggerPluginLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("Extensions");
    return STATIC_LOGGER;
}