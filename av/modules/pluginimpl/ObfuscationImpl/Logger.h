/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getObfuscationLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getObfuscationLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getObfuscationLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getObfuscationLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getObfuscationLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getObfuscationLogger(), x)     // NOLINT
