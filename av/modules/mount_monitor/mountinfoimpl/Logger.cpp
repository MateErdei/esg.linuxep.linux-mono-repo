/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"

#include "common/config.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/ConsoleFileLoggingSetup.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getMountInfoImplLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("NamedScanRunner");
    return STATIC_LOGGER;
}
