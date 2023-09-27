/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSessionRunnerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSessionRunnerLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSessionRunnerLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSessionRunnerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSessionRunnerLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSessionRunnerLogger(), x)     // NOLINT
