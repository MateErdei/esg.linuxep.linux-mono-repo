// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getRemoteDiagnoseLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getRemoteDiagnoseLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getRemoteDiagnoseLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getRemoteDiagnoseLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getRemoteDiagnoseLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getRemoteDiagnoseLogger(), x)
