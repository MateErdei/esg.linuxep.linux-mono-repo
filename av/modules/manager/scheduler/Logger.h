// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getScanSchedulerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getScanSchedulerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getScanSchedulerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getScanSchedulerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getScanSchedulerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getScanSchedulerLogger(), x)
