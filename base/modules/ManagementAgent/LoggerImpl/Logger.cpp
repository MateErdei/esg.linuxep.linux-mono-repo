/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <log4cplus/logger.h>
#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getManagementAgentLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("managementagent");
    return STATIC_LOGGER;
}
