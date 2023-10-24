// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getOsqueryLogger();

#define LOGDEBUG_OSQUERY(x) LOG4CPLUS_DEBUG(getOsqueryLogger(), x)  // NOLINT
#define LOGINFO_OSQUERY(x) LOG4CPLUS_INFO(getOsqueryLogger(), x)    // NOLINT
#define LOGSUPPORT_OSQUERY(x) LOG4CPLUS_SUPPORT(getOsqueryLogger(), x) // NOLINT
#define LOGWARN_OSQUERY(x) LOG4CPLUS_WARN(getOsqueryLogger(), x)    // NOLINT
#define LOGERROR_OSQUERY(x) LOG4CPLUS_ERROR(getOsqueryLogger(), x)  // NOLINT
