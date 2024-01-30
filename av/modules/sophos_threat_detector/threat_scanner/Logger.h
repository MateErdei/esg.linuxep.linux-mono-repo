// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getThreatScannerLogger();
log4cplus::Logger& getSusiDebugLogger();

#define LOGTRACE(x) LOG4CPLUS_TRACE(getThreatScannerLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getThreatScannerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getThreatScannerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getThreatScannerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getThreatScannerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getThreatScannerLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getThreatScannerLogger(), x)

#define LOG_SUSI_TRACE(x) LOG4CPLUS_TRACE(getSusiDebugLogger(), x)
#define LOG_SUSI_DEBUG(x) LOG4CPLUS_DEBUG(getSusiDebugLogger(), x)
#define LOG_SUSI_SUPPORT(x) LOG4CPLUS_SUPPORT(getSusiDebugLogger(), x)
#define LOG_SUSI_INFO(x) LOG4CPLUS_INFO(getSusiDebugLogger(), x)
#define LOG_SUSI_WARN(x) LOG4CPLUS_WARN(getSusiDebugLogger(), x)
#define LOG_SUSI_ERROR(x) LOG4CPLUS_ERROR(getSusiDebugLogger(), x)