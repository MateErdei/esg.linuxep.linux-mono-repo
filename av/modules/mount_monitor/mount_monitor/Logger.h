/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

#include <string>

log4cplus::Logger& getMountMonitorLogger();

#define LOGTRACE(x) LOG4CPLUS_TRACE(getMountMonitorLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getMountMonitorLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getMountMonitorLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getMountMonitorLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getMountMonitorLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getMountMonitorLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getMountMonitorLogger(), x)

