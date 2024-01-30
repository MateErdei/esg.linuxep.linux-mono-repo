// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getOnAccessImplLogger();

#ifndef USING_LIBFUZZER
#define LOGTRACE(x) LOG4CPLUS_TRACE(getOnAccessImplLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getOnAccessImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getOnAccessImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getOnAccessImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getOnAccessImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getOnAccessImplLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getOnAccessImplLogger(), x)
#else
//Discard logs in fuzz mode
# include "common/Logger.h"
#endif