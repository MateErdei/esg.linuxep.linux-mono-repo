// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getQueryRunnerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getQueryRunnerLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getQueryRunnerLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getQueryRunnerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getQueryRunnerLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getQueryRunnerLogger(), x)     // NOLINT
