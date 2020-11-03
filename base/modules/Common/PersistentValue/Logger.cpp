/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getPersistentValueLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("PersistentValue");
    return STATIC_LOGGER;
}
