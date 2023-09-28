// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef USING_LIBFUZZER

#define LOG(x) do {} while(0) // NOLINT

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

log4cplus::Logger& getPluginLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getPluginLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginLogger(), x)  // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getPluginLogger(), x)  // NOLINT

#endif