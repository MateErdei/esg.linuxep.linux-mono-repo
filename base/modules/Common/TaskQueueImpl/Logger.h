/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getTaskQueueImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getTaskQueueImplLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getTaskQueueImplLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getTaskQueueImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getTaskQueueImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getTaskQueueImplLogger(), x)  // NOLINT
