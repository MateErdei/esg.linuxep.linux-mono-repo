// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getLiveQueryLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getLiveQueryLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getLiveQueryLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getLiveQueryLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getLiveQueryLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getLiveQueryLogger(), x)     // NOLINT
