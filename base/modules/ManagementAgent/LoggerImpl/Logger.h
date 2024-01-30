// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getManagementAgentLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getManagementAgentLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getManagementAgentLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getManagementAgentLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getManagementAgentLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getManagementAgentLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getManagementAgentLogger(), x)
