// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getRegisterCentralLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getRegisterCentralLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getRegisterCentralLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getRegisterCentralLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getRegisterCentralLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getRegisterCentralLogger(), x)
