// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getActionsMainLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getActionsMainLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getActionsMainLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getActionsMainLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getActionsMainLogger(), x)
