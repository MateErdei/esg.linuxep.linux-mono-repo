// Copyright 2021 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getEventJournalLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getEventJournalLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getEventJournalLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getEventJournalLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getEventJournalLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getEventJournalLogger(), x)
