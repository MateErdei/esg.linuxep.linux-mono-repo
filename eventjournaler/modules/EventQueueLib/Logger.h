// Copyright 2021 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getEventQueueLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getEventQueueLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getEventQueueLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getEventQueueLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getEventQueueLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getEventQueueLogger(), x)
