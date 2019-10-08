/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getDiagnoseLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getDiagnoseLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getDiagnoseLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getDiagnoseLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getDiagnoseLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getDiagnoseLogger(), x)     // NOLINT
