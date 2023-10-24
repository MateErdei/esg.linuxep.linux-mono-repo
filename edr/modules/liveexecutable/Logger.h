// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getLiveQueryMainLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getLiveQueryMainLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getLiveQueryMainLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getLiveQueryMainLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getLiveQueryMainLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getLiveQueryMainLogger(), x)     // NOLINT
