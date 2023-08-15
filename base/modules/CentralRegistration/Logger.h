// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getRegisterCentralLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getRegisterCentralLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getRegisterCentralLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getRegisterCentralLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getRegisterCentralLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getRegisterCentralLogger(), x)     // NOLINT
