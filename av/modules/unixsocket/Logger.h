/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getUnixSocketLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getUnixSocketLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getUnixSocketLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getUnixSocketLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getUnixSocketLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getUnixSocketLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getUnixSocketLogger(), x)  // NOLINT
