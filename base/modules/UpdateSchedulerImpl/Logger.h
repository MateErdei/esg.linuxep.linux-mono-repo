/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getUpdateSchedulerImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getUpdateSchedulerImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getUpdateSchedulerImplLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getUpdateSchedulerImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getUpdateSchedulerImplLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getUpdateSchedulerImplLogger(), x) // NOLINT
