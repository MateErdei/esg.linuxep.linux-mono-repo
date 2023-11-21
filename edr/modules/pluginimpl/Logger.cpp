// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "Logger.h"

#include "livequery/config.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getPluginLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance(PLUGIN_NAME);
    return STATIC_LOGGER;
}
