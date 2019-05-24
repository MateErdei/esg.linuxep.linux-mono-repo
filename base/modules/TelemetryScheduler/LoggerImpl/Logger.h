/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getTelemetrySchedulerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTelemetrySchedulerLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getTelemetrySchedulerLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getTelemetrySchedulerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getTelemetrySchedulerLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getTelemetrySchedulerLogger(), x)     // NOLINT
