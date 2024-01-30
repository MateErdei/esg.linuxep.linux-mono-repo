// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getWatchdogRequestImplLogger();

#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getWatchdogRequestImplLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWatchdogRequestImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getWatchdogRequestImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getWatchdogRequestImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getWatchdogRequestImplLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getWatchdogRequestImplLogger(), x)
