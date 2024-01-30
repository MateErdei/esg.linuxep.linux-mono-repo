// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getTaskQueueImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTaskQueueImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getTaskQueueImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getTaskQueueImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getTaskQueueImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getTaskQueueImplLogger(), x)
