/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getTelemetryLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTelemetryLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getTelemetryLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getTelemetryLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getTelemetryLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getTelemetryLogger(), x)     // NOLINT
