/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getWdctlLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWdctlLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getWdctlLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getWdctlLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getWdctlLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getWdctlLogger(), x)     // NOLINT
