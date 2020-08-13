/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSophosThreadDetectorImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSophosThreadDetectorImplLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSophosThreadDetectorImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSophosThreadDetectorImplLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSophosThreadDetectorImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSophosThreadDetectorImplLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSophosThreadDetectorImplLogger(), x)  // NOLINT
