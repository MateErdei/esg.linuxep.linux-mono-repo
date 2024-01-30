// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getObfuscationLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getObfuscationLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getObfuscationLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getObfuscationLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getObfuscationLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getObfuscationLogger(), x)
