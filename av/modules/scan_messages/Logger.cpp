// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "Logger.h"

#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getScanMessagesLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("scan_messages");
    return STATIC_LOGGER;
}
