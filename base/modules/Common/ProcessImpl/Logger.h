/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getProcessImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProcessImplLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getProcessImplLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getProcessImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getProcessImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getProcessImplLogger(), x)  // NOLINT
