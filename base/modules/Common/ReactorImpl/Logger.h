// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getReactorLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getReactorLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getReactorLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getReactorLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getReactorLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getReactorLogger(), x)
