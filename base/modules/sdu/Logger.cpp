// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "Common/Logging/LoggerConfig.h"
#include <log4cplus/logger.h>

log4cplus::Logger& getRemoteDiagnoseLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("remote_diagnose");
    return STATIC_LOGGER;
}
