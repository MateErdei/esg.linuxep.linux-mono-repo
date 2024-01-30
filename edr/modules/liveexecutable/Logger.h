// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getLiveQueryMainLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getLiveQueryMainLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getLiveQueryMainLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getLiveQueryMainLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getLiveQueryMainLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getLiveQueryMainLogger(), x)
