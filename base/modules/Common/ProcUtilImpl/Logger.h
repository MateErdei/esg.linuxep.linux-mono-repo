/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getProcLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getProcLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getProcLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getProcLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getProcLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getProcLogger(), x) // NOLINT
