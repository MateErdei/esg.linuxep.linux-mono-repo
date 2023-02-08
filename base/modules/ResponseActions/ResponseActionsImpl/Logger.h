// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getResponseActionsImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getResponseActionsImplLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getResponseActionsImplLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getResponseActionsImplLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getResponseActionsImplLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getResponseActionsImplLogger(), x)  // NOLINT
