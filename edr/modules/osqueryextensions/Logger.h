// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getLoggerPluginLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getLoggerPluginLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getLoggerPluginLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getLoggerPluginLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getLoggerPluginLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getLoggerPluginLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getLoggerPluginLogger(), x)