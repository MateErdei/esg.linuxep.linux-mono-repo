/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

#include <string>

log4cplus::Logger& getMountMonitorLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getMountMonitorLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getMountMonitorLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getMountMonitorLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getMountMonitorLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getMountMonitorLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getMountMonitorLogger(), x)  // NOLINT

