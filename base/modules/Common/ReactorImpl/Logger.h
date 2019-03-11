/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getReactorLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getReactorLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getReactorLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getReactorLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getReactorLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getReactorLogger(), x)  // NOLINT
