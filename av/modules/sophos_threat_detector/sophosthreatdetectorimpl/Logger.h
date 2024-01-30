// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getSophosThreatDetectorImplLogger();

#define LOGTRACE(x) LOG4CPLUS_TRACE(getSophosThreatDetectorImplLogger(), x)
#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSophosThreatDetectorImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSophosThreatDetectorImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getSophosThreatDetectorImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getSophosThreatDetectorImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSophosThreatDetectorImplLogger(), x)
#define LOGFATAL(x) LOG4CPLUS_FATAL(getSophosThreatDetectorImplLogger(), x)
