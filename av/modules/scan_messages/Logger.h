// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getScanMessagesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getScanMessagesLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getScanMessagesLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getScanMessagesLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getScanMessagesLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getScanMessagesLogger(), x)
