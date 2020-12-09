/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getWdctlActionsLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWdctlActionsLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getWdctlActionsLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getWdctlActionsLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getWdctlActionsLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getWdctlActionsLogger(), x) // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getWdctlActionsLogger(), x) // NOLINT