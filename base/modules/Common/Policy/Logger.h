// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getPolicyLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPolicyLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPolicyLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPolicyLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPolicyLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPolicyLogger(), x) // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getPolicyLogger(), x) // NOLINT

