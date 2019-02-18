/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getWdctlLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWdctlLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getWdctlLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getWdctlLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getWdctlLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getWdctlLogger(), x)  // NOLINT
