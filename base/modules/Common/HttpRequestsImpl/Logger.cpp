/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"
log4cplus::Logger& getHttpRequesterImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("HttpRequester");
    return STATIC_LOGGER;
} // LCOV_EXCL_LINE