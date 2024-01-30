// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getTelemetrySchedulerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTelemetrySchedulerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getTelemetrySchedulerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getTelemetrySchedulerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getTelemetrySchedulerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getTelemetrySchedulerLogger(), x)
