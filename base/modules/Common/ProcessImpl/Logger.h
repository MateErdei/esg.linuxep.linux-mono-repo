// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getProcessImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProcessImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getProcessImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getProcessImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getProcessImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getProcessImplLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getProcessImplLogger(), x)
