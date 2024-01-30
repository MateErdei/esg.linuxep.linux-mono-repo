// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getQueryRunnerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getQueryRunnerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getQueryRunnerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getQueryRunnerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getQueryRunnerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getQueryRunnerLogger(), x)
