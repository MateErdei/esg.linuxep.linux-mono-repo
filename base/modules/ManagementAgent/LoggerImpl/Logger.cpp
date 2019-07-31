/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/LoggerConfig.h>
#include <log4cplus/logger.h>

log4cplus::Logger& getManagementAgentLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("managementagent");
    return STATIC_LOGGER;
}
