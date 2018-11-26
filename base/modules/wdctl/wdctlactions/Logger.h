/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getWdctlActionsLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWdctlActionsLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getWdctlActionsLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getWdctlActionsLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getWdctlActionsLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getWdctlActionsLogger(), x) // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getWdctlActionsLogger(), x) // NOLINT