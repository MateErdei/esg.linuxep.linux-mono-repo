// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getSafeStoreLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("safestore");
    return STATIC_LOGGER;
}