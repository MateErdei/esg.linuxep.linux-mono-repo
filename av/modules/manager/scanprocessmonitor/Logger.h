/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

namespace plugin::manager::scanprocessmonitor
{
    log4cplus::Logger& getScanProcessMonitorLogger();
}

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getScanProcessMonitorLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getScanProcessMonitorLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getScanProcessMonitorLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getScanProcessMonitorLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getScanProcessMonitorLogger(), x)  // NOLINT
