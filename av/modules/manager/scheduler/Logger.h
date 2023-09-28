// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getScanSchedulerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getScanSchedulerLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getScanSchedulerLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getScanSchedulerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getScanSchedulerLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getScanSchedulerLogger(), x)  // NOLINT
