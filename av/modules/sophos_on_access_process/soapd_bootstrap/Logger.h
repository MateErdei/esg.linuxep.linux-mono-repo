/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSophosOnAccessBootstrapImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSophosOnAccessBootstrapImplLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSophosOnAccessBootstrapImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSophosOnAccessBootstrapImplLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSophosOnAccessBootstrapImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSophosOnAccessBootstrapImplLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getSophosOnAccessBootstrapImplLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSophosOnAccessBootstrapImplLogger(), x)  // NOLINT
