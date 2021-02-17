/******************************************************************************************************

Copyright 2018-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getScheduledQueryLogger();

#define LOGDEBUG_SQ(x) LOG4CPLUS_DEBUG(getScheduledQueryLogger(), x)     // NOLINT
#define LOGINFO_SQ(x) LOG4CPLUS_INFO(getScheduledQueryLogger(), x)       // NOLINT
#define LOGSUPPORT_SQ(x) LOG4CPLUS_SUPPORT(getScheduledQueryLogger(), x) // NOLINT
#define LOGWARN_SQ(x) LOG4CPLUS_WARN(getScheduledQueryLogger(), x)       // NOLINT
#define LOGERROR_SQ(x) LOG4CPLUS_ERROR(getScheduledQueryLogger(), x)     // NOLINT