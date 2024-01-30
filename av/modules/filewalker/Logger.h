// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getFileWalkerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getFileWalkerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getFileWalkerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getFileWalkerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getFileWalkerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getFileWalkerLogger(), x)
