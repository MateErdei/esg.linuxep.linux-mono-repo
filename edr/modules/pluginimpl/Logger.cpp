/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include "config.h"

log4cplus::Logger& getPluginLogger()
{
    static log4cplus::Logger STATIC_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(PLUGIN_NAME));
    return STATIC_LOGGER;
}
