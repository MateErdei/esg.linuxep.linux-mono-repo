// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getProcessMonitoringImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProcessMonitoringImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getProcessMonitoringImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getProcessMonitoringImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getProcessMonitoringImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getProcessMonitoringImplLogger(), x)
