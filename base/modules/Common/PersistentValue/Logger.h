/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getPersistentValueLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPersistentValueLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPersistentValueLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPersistentValueLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPersistentValueLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPersistentValueLogger(), x)     // NOLINT
