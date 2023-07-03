// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getProxyUtilsLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProxyUtilsLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getProxyUtilsLogger(), x)   // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getProxyUtilsLogger(), x)   // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getProxyUtilsLogger(), x) // NOLINT
