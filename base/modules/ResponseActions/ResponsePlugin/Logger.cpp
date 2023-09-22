// Copyright 2023 Sophos Limited. All rights reserved.
#include "Logger.h"

#include "ResponseActions/ResponsePlugin/config.h"
#include "Common/Logging/LoggerConfig.h"
#include <log4cplus/logger.h>

log4cplus::Logger& getPluginLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance(RA_PLUGIN_NAME);
    return STATIC_LOGGER;
}
