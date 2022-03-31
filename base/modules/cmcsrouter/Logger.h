/******************************************************************************************************
Copyright 2022, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getMCSImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getHttpRequesterImplLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getHttpRequesterImplLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getHttpRequesterImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getHttpRequesterImplLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getHttpRequesterImplLogger(), x)     // NOLINT
