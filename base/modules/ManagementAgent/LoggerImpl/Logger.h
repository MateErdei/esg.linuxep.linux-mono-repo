/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getManagementAgentLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getManagementAgentLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getManagementAgentLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getManagementAgentLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getManagementAgentLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getManagementAgentLogger(), x) // NOLINT

