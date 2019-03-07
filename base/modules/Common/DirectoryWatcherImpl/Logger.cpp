/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include <Common/Logging/LoggerConfig.h>
log4cplus::Logger& getDirectoryWatcherImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("DirectoryWatcherImpl");
    return STATIC_LOGGER;
}