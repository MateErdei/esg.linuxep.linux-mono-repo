// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getDirectoryWatcherImplLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getDirectoryWatcherImplLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getDirectoryWatcherImplLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getDirectoryWatcherImplLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getDirectoryWatcherImplLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getDirectoryWatcherImplLogger(), x)
