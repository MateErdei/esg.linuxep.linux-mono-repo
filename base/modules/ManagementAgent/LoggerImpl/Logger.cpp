/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <log4cplus/logger.h>

log4cplus::Logger& getManagementAgentLogger()
{
    static log4cplus::Logger STATIC_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("managementagent"));
    return STATIC_LOGGER;
}
