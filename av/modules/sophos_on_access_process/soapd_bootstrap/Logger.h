/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSophosOnAccessImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSophosOnAccessImplLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSophosOnAccessImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSophosOnAccessImplLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSophosOnAccessImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSophosOnAccessImplLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSophosOnAccessImplLogger(), x)  // NOLINT
