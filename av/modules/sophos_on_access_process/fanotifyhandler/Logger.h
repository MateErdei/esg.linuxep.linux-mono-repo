// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getFaNotifyHandlerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getFaNotifyHandlerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getFaNotifyHandlerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getFaNotifyHandlerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getFaNotifyHandlerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getFaNotifyHandlerLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getFaNotifyHandlerLogger(), x)
#define LOGTRACE(x) LOG4CPLUS_TRACE(getFaNotifyHandlerLogger(), x)
