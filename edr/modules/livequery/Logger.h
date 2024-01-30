// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getLiveQueryLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getLiveQueryLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getLiveQueryLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getLiveQueryLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getLiveQueryLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getLiveQueryLogger(), x)
