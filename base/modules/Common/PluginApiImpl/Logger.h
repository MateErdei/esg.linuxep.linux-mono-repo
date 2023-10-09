// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getPluginAPILogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginAPILogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginAPILogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginAPILogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginAPILogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginAPILogger(), x)     // NOLINT
