/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

#include <string>

log4cplus::Logger& getMountInfoImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getMountInfoImplLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getMountInfoImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getMountInfoImplLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getMountInfoImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getMountInfoImplLogger(), x)  // NOLINT

