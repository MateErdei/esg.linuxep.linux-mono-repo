// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getMCSImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getMCSImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getMCSImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getMCSImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getMCSImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getMCSImplLogger(), x)
