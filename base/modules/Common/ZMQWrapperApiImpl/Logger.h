// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getZeroMQWrapperApiLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getZeroMQWrapperApiLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getZeroMQWrapperApiLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getZeroMQWrapperApiLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getZeroMQWrapperApiLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getZeroMQWrapperApiLogger(), x)
