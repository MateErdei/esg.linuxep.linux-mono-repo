/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getCommsComponentLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getCommsComponentLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getCommsComponentLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getCommsComponentLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getCommsComponentLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getCommsComponentLogger(), x)     // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getCommsComponentLogger(), x)     // NOLINT
