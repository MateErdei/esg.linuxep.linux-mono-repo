// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getScheduledQueryLogger()
{
    static log4cplus::Logger QUERY_STATIC_LOGGER = Common::Logging::getInstance("scheduledquery");
    return QUERY_STATIC_LOGGER;
}