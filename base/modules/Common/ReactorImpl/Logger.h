/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <iostream>

log4cplus::Logger& getReactorLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getReactorLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getReactorLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getReactorLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getReactorLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getReactorLogger(), x)  // NOLINT
