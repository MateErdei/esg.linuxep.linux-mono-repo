// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef USING_LIBFUZZER

#define LOG(x) do {} while(0)

#include "datatypes/Print.h"
//#define LOG(x) PRINT(x)

#define LOGTRACE(X)   LOG(x)
#define LOGDEBUG(x)   LOG(x)
#define LOGINFO(x)    LOG(x)
#define LOGSUPPORT(x) LOG(x)
#define LOGWARN(x)    LOG(x)
#define LOGERROR(x)   LOG(x)
#define LOGFATAL(x)   do { PRINT(x); ::exit(1); } while(0)

#else
#define LOGGING_ENABLED

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getCommonLogger();

#define LOGTRACE(x) LOG4CPLUS_TRACE(getCommonLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getCommonLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getCommonLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getCommonLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getCommonLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getCommonLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getCommonLogger(), x)

#endif