/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>
log4cplus::Logger& getHttpSenderImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("HttpSenderImpl");
    return STATIC_LOGGER;
} // LCOV_EXCL_LINE