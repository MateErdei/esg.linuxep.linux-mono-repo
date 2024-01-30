// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getHttpRequesterImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getHttpRequesterImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getHttpRequesterImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getHttpRequesterImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getHttpRequesterImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getHttpRequesterImplLogger(), x)
