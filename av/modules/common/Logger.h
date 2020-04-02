/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getFileWalkerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getFileWalkerLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getFileWalkerLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getFileWalkerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getFileWalkerLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getFileWalkerLogger(), x)  // NOLINT
