// Copyright 2021 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getSubscriberLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSubscriberLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSubscriberLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSubscriberLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSubscriberLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSubscriberLogger(), x)     // NOLINT
