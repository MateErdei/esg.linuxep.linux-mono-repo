/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getCronSchedulerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getCronSchedulerLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getCronSchedulerLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getCronSchedulerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getCronSchedulerLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getCronSchedulerLogger(), x)  // NOLINT
