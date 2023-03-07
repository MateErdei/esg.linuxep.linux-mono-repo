// Copyright 2023 Sophos Limited. All rights reserved.
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getProxyUtilsLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("proxyutils");
    return STATIC_LOGGER;
}
