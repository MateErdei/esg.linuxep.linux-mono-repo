// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getEventWriterLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getEventWriterLogger(), x)     // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getEventWriterLogger(), x)       // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getEventWriterLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getEventWriterLogger(), x)       // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getEventWriterLogger(), x)     // NOLINT
