// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getStateMachineLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getStateMachineLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getStateMachineLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getStateMachineLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getStateMachineLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getStateMachineLogger(), x)
