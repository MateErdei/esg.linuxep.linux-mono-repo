// Copyright 2021 Sophos Limited. All rights reserved.

#include "Common/Logging/LoggerConfig.h"

#include "Logger.h"

log4cplus::Logger& getEventJournalLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("EventJournal");
    return STATIC_LOGGER;
}
