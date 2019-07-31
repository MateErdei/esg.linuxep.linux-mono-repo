/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getTelemetryHelperLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTelemetryHelperLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getTelemetryHelperLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getTelemetryHelperLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getTelemetryHelperLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getTelemetryHelperLogger(), x)     // NOLINT
