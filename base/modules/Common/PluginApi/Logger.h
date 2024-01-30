// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getPluginAPIInterfaceLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginAPIInterfaceLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginAPIInterfaceLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginAPIInterfaceLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginAPIInterfaceLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginAPIInterfaceLogger(), x)
