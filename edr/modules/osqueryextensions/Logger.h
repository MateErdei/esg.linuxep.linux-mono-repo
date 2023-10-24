// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getLoggerPluginLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getLoggerPluginLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getLoggerPluginLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getLoggerPluginLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getLoggerPluginLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getLoggerPluginLogger(), x)     // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getLoggerPluginLogger(), x)     // NOLINT