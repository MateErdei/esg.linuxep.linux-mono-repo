// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getProcLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProcLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getProcLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getProcLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getProcLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getProcLogger(), x)
