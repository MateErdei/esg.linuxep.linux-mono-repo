/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getThreatScannerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getThreatScannerLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getThreatScannerLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getThreatScannerLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getThreatScannerLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getThreatScannerLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getThreatScannerLogger(), x)  // NOLINT
