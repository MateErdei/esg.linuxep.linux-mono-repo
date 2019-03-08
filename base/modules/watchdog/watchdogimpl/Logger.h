/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getWatchdogImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWatchdogImplLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getWatchdogImplLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getWatchdogImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getWatchdogImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getWatchdogImplLogger(), x)  // NOLINT
