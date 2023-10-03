// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getEventWriterLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("EventWriter");
    return STATIC_LOGGER;
}
