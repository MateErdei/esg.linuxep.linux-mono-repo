// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getTelemetryLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTelemetryLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getTelemetryLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getTelemetryLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getTelemetryLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getTelemetryLogger(), x)