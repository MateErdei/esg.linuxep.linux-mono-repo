// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getSafeStoreLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSafeStoreLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getSafeStoreLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getSafeStoreLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSafeStoreLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getSafeStoreLogger(), x)
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSafeStoreLogger(), x)
