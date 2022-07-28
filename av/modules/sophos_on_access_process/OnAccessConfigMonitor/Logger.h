/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getOnAccessConfigMonitorLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getOnAccessConfigMonitorLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getOnAccessConfigMonitorLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getOnAccessConfigMonitorLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getOnAccessConfigMonitorLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getOnAccessConfigMonitorLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getOnAccessConfigMonitorLogger(), x)  // NOLINT
