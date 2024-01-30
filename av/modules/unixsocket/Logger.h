// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef USING_LIBFUZZER

#define LOG(x) do {} while(0)

#include "datatypes/Print.h"
//#define LOG(x) PRINT(x)

#define LOGDEBUG(x) LOG(x)
#define LOGINFO(x) LOG(x)
#define LOGSUPPORT(x) LOG(x)
#define LOGWARN(x) LOG(x)
#define LOGERROR(x) LOG(x)
#define LOGFATAL(x) do { PRINT(x); ::exit(1); } while(0)

#else

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getUnixSocketLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getUnixSocketLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getUnixSocketLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getUnixSocketLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getUnixSocketLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getUnixSocketLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getUnixSocketLogger(), x)

#endif