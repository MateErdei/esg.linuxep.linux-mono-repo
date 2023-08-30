// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getUpdateUtilitiesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getUpdateUtilitiesLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getUpdateUtilitiesLogger(), x)   // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getUpdateUtilitiesLogger(), x)   // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getUpdateUtilitiesLogger(), x) // NOLINT
