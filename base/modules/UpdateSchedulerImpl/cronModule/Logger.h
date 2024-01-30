// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getCronSchedulerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getCronSchedulerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getCronSchedulerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getCronSchedulerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getCronSchedulerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getCronSchedulerLogger(), x)
