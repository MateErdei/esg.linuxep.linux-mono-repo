// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getScheduledQueryLogger();

#define LOGDEBUG_SCHEDULEDOSQUERY(x) LOG4CPLUS_DEBUG(getScheduledQueryLogger(), x)     // NOLINT
#define LOGINFO_SCHEDULEDOSQUERY(x) LOG4CPLUS_INFO(getScheduledQueryLogger(), x)       // NOLINT
#define LOGSUPPORT_SCHEDULEDOSQUERY(x) LOG4CPLUS_SUPPORT(getScheduledQueryLogger(), x) // NOLINT
#define LOGWARN_SCHEDULEDOSQUERY(x) LOG4CPLUS_WARN(getScheduledQueryLogger(), x)       // NOLINT
#define LOGERROR_SCHEDULEDOSQUERY(x) LOG4CPLUS_ERROR(getScheduledQueryLogger(), x)     // NOLINT