/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getStateMachineLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getStateMachineLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getStateMachineLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getStateMachineLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getStateMachineLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getStateMachineLogger(), x)     // NOLINT
