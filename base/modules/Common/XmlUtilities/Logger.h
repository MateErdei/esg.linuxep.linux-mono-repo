/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getLogger(), x) // NOLINT
