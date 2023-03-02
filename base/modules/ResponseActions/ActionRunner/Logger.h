// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getActionsMainLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getActionsMainLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getActionsMainLogger(), x)       // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getActionsMainLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getActionsMainLogger(), x)     // NOLINT
