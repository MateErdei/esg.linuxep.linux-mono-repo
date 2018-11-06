/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

log4cplus::Logger& getPluginLogger()
{
    static log4cplus::Logger STATIC_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("exampleplugin"));
    return STATIC_LOGGER;
}
