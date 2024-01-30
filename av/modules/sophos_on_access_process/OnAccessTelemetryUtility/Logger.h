// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getOnAccessTelemetryUtilityLogger();

#ifndef USING_LIBFUZZER
#define LOGTRACE(x) LOG4CPLUS_TRACE(getOnAccessTelemetryUtilityLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getOnAccessTelemetryUtilityLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getOnAccessTelemetryUtilityLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getOnAccessTelemetryUtilityLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getOnAccessTelemetryUtilityLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getOnAccessTelemetryUtilityLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getOnAccessTelemetryUtilityLogger(), x)
#else
//Discard logs in fuzz mode
# include "common/Logger.h"
#endif