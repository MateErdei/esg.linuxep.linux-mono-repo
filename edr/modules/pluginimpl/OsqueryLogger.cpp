// Copyright 2020-2023 Sophos Limited. All rights reserved.
#include "OsqueryLogger.h"
#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getOsqueryLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("edr_osquery");
    return STATIC_LOGGER;
}
