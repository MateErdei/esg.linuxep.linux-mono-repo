// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getWatchdogImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWatchdogImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getWatchdogImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getWatchdogImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getWatchdogImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getWatchdogImplLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getWatchdogImplLogger(), x)
