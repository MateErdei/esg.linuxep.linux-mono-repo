/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getDirectoryWatcherImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getDirectoryWatcherImplLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getDirectoryWatcherImplLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getDirectoryWatcherImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getDirectoryWatcherImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getDirectoryWatcherImplLogger(), x)  // NOLINT
