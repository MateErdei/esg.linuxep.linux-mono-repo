/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getXmlUtilitiesLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("XmlUtilities");
    return STATIC_LOGGER;
}
