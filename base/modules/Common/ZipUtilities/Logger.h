// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getZipUtilitiesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getZipUtilitiesLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getZipUtilitiesLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getZipUtilitiesLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getZipUtilitiesLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getZipUtilitiesLogger(), x) // NOLINT
