// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getZipUtilitiesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getZipUtilitiesLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getZipUtilitiesLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getZipUtilitiesLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getZipUtilitiesLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getZipUtilitiesLogger(), x)
