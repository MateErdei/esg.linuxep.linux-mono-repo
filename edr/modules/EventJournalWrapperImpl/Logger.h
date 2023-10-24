// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getEventJournalWrapperLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getEventJournalWrapperLogger(), x)      // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getEventJournalWrapperLogger(), x)        // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getEventJournalWrapperLogger(), x)  // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getEventJournalWrapperLogger(), x)        // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getEventJournalWrapperLogger(), x)      // NOLINT
