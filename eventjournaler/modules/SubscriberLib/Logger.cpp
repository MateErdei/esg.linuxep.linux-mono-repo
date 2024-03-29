// Copyright 2021 Sophos Limited. All rights reserved.
#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getSubscriberLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("subscriber");
    return STATIC_LOGGER;
}
