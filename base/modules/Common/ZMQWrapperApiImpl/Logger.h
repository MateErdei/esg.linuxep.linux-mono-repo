/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getZeroMQWrapperApiLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getZeroMQWrapperApiLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getZeroMQWrapperApiLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getZeroMQWrapperApiLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getZeroMQWrapperApiLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getZeroMQWrapperApiLogger(), x)  // NOLINT
