/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getOnAccessConfigLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getOnAccessConfigLogger(), x)  // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getOnAccessConfigLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getOnAccessConfigLogger(), x)    // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getOnAccessConfigLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getOnAccessConfigLogger(), x)  // NOLINT
#define LOGTRACE(x) LOG4CPLUS_TRACE(getOnAccessConfigLogger(), x)  // NOLINT
