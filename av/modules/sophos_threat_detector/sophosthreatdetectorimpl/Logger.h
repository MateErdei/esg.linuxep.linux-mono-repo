/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSophosThreatDetectorImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSophosThreatDetectorImplLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSophosThreatDetectorImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSophosThreatDetectorImplLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSophosThreatDetectorImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSophosThreatDetectorImplLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSophosThreatDetectorImplLogger(), x)  // NOLINT
