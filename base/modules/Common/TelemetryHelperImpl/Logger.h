// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getTelemetryHelperLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTelemetryHelperLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getTelemetryHelperLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getTelemetryHelperLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getTelemetryHelperLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getTelemetryHelperLogger(), x)
