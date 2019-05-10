/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getProcessMonitoringImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProcessMonitoringImplLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getProcessMonitoringImplLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getProcessMonitoringImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getProcessMonitoringImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getProcessMonitoringImplLogger(), x)  // NOLINT
