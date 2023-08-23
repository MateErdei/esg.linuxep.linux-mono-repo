// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getFlagUtilsLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getFlagUtilsLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getFlagUtilsLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getFlagUtilsLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getFlagUtilsLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getFlagUtilsLogger(), x)     // NOLINT
