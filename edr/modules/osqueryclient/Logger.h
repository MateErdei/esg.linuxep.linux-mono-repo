// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getOSQueryClientLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getOSQueryClientLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getOSQueryClientLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getOSQueryClientLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getOSQueryClientLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getOSQueryClientLogger(), x)
