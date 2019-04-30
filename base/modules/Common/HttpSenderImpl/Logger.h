/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getHttpSenderImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getHttpSenderImplLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getHttpSenderImplLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getHttpSenderImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getHttpSenderImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getHttpSenderImplLogger(), x)  // NOLINT
