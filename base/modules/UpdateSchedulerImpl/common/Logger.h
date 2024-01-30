// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getUpdateSchedulerImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getUpdateSchedulerImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getUpdateSchedulerImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getUpdateSchedulerImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getUpdateSchedulerImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getUpdateSchedulerImplLogger(), x)
