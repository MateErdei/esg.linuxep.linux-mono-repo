//Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getFaNotifyHandlerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getFaNotifyHandlerLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getFaNotifyHandlerLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getFaNotifyHandlerLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getFaNotifyHandlerLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getFaNotifyHandlerLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getFaNotifyHandlerLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getFaNotifyHandlerLogger(), x)  // NOLINT
