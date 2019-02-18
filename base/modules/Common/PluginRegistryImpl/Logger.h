/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getPluginRegistryLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginRegistryLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginRegistryLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getPluginRegistryLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginRegistryLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginRegistryLogger(), x)  // NOLINT
