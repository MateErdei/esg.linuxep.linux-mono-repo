// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getPluginRegistryLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginRegistryLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginRegistryLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginRegistryLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginRegistryLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginRegistryLogger(), x)
