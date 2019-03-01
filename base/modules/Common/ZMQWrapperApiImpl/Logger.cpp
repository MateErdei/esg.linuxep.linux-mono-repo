/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

log4cplus::Logger& getZeroMQWrapperApiLogger()
{
    static log4cplus::Logger STATIC_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("ZeroMQWrapperApi"));
    return STATIC_LOGGER;
}
