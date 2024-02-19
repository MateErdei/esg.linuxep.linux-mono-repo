// Copyright 2024 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getSafeStoreExtractorLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSafeStoreExtractorLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getSafeStoreExtractorLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getSafeStoreExtractorLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSafeStoreExtractorLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getSafeStoreExtractorLogger(), x)
#define LOGTRACE(x) LOG4CPLUS_TRACE(getSafeStoreExtractorLogger(), x)
