/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getPluginAPILogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginAPILogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginAPILogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getPluginAPILogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginAPILogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginAPILogger(), x)  // NOLINT
