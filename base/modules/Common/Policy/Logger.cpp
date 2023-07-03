// Copyright 2023 Sophos All rights reserved.

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getPolicyLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("Policy");
    return STATIC_LOGGER;
}
