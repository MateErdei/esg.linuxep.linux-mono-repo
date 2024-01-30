// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getProxyUtilsLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProxyUtilsLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getProxyUtilsLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getProxyUtilsLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getProxyUtilsLogger(), x)
