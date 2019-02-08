/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getPluginLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getPluginLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginLogger(), x)  // NOLINT
