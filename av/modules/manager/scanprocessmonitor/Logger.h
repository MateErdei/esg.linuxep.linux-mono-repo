/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getScanProcessMonitor();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getScanProcessMonitor(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getScanProcessMonitor(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getScanProcessMonitor(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getScanProcessMonitor(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getScanProcessMonitor(), x)  // NOLINT
