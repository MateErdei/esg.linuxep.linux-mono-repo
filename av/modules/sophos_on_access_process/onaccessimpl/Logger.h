// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getOnAccessImplLogger();

#ifndef USING_LIBFUZZER
#define LOGTRACE(x) LOG4CPLUS_TRACE(getOnAccessImplLogger(), x)  // NOLINT
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getOnAccessImplLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getOnAccessImplLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getOnAccessImplLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getOnAccessImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getOnAccessImplLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getOnAccessImplLogger(), x)  // NOLINT
#else
//Discard logs in fuzz mode
# include "common/Logger.h"
#endif