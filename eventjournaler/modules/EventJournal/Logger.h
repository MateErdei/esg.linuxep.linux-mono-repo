// Copyright 2021 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getEventJournalLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getEventJournalLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getEventJournalLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getEventJournalLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getEventJournalLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getEventJournalLogger(), x)     // NOLINT
