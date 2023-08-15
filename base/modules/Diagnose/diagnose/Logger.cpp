// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getDiagnoseLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("diagnose");
    return STATIC_LOGGER;
}
