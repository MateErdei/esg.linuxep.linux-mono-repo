// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

#include <log4cplus/logger.h>

log4cplus::Logger& getUpdateUtilitiesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getUpdateUtilitiesLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getUpdateUtilitiesLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getUpdateUtilitiesLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getUpdateUtilitiesLogger(), x)
