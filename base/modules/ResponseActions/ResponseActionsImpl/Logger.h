// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getResponseActionsImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getResponseActionsImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getResponseActionsImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getResponseActionsImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getResponseActionsImplLogger(), x)
