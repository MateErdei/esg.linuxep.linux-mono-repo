/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSafestoreLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSafestoreLogger(), x)     // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSafestoreLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSafestoreLogger(), x)       // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSafestoreLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSafestoreLogger(), x)     // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getSafestoreLogger(), x)     // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSafestoreLogger(), x)     // NOLINT
