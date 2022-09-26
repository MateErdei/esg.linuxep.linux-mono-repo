// Copyright 2018-2022, Sophos Limited.  All rights reserved.

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getPluginLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getPluginLogger(), x)  // NOLINT
