// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getPolicyLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPolicyLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getPolicyLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPolicyLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getPolicyLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getPolicyLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getPolicyLogger(), x)
