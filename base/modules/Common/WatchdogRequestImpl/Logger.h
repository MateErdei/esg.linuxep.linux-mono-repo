// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getWatchdogRequestImplLogger();

#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getWatchdogRequestImplLogger(), x) // NOLINT
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWatchdogRequestImplLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getWatchdogRequestImplLogger(), x)       // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getWatchdogRequestImplLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getWatchdogRequestImplLogger(), x)     // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getWatchdogRequestImplLogger(), x)     // NOLINT
