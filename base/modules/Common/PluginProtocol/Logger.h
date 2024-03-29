// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getPluginAPIProtocolLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginAPIProtocolLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginAPIProtocolLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginAPIProtocolLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginAPIProtocolLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginAPIProtocolLogger(), x)
