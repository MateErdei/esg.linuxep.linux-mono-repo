/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getPluginRegistryLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginRegistryLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginRegistryLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginRegistryLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginRegistryLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginRegistryLogger(), x) // NOLINT
