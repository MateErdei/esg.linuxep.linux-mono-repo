// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getSessionRunnerLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("sessionrunner");
    return STATIC_LOGGER;
}
