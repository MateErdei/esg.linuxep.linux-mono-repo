// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSafeStoreLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSafeStoreLogger(), x)     // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSafeStoreLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSafeStoreLogger(), x)       // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSafeStoreLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSafeStoreLogger(), x)     // NOLINT
#define LOGFATAL(x) LOG4CPLUS_FATAL(getSafeStoreLogger(), x)     // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSafeStoreLogger(), x)     // NOLINT
