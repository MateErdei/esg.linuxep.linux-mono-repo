/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include "modules/Common/Logging/LoggerConfig.h"

log4cplus::Logger& getPluginAPIProtocolLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("pluginapi");
    return STATIC_LOGGER;
}
