// Copyright 2022-2023 Sophos Limited. All rights reserved.

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

log4cplus::Logger& getSafeStoreSocketLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSafeStoreSocketLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getSafeStoreSocketLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSafeStoreSocketLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getSafeStoreSocketLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSafeStoreSocketLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getSafeStoreSocketLogger(), x)

#endif
