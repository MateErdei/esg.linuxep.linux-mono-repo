/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getRemoteDiagnoseLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getRemoteDiagnoseLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getRemoteDiagnoseLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getRemoteDiagnoseLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getRemoteDiagnoseLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getRemoteDiagnoseLogger(), x)     // NOLINT
