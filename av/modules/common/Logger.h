/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getCommonLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getCommonLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getCommonLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getCommonLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getCommonLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getCommonLogger(), x)  // NOLINT
