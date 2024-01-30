// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getScanRequestQueueLogger();

#ifndef USING_LIBFUZZER
#define LOGTRACE(x) LOG4CPLUS_TRACE(getScanRequestQueueLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getScanRequestQueueLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getScanRequestQueueLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getScanRequestQueueLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getScanRequestQueueLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getScanRequestQueueLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getScanRequestQueueLogger(), x)
#else
//Discard logs in fuzz mode
# include "common/Logger.h"
#endif