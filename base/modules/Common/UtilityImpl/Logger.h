/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getUtilityImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getUtilityImplLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getUtilityImplLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getUtilityImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getUtilityImplLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getUtilityImplLogger(), x)     // NOLINT
