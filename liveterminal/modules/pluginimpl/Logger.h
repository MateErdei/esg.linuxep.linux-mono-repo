// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getPluginLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginLogger(), x)
