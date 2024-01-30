// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getDiagnoseLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getDiagnoseLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getDiagnoseLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getDiagnoseLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getDiagnoseLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getDiagnoseLogger(), x)
