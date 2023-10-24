// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "Common/Logging/LoggerConfig.h"

#include "Logger.h"

log4cplus::Logger& getEventJournalWrapperLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("EventJournalWrapperImpl");
    return STATIC_LOGGER;
}
