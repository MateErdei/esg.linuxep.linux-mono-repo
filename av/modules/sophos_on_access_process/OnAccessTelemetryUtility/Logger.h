// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getOnAccessTelemetryUtilityLogger();

#ifndef USING_LIBFUZZER
#define LOGTRACE(x) LOG4CPLUS_TRACE(getOnAccessTelemetryUtilityLogger(), x)  // NOLINT
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getOnAccessTelemetryUtilityLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getOnAccessTelemetryUtilityLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getOnAccessTelemetryUtilityLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getOnAccessTelemetryUtilityLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getOnAccessTelemetryUtilityLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getOnAccessTelemetryUtilityLogger(), x)  // NOLINT
#else
//Discard logs in fuzz mode
# include "common/Logger.h"
#endif