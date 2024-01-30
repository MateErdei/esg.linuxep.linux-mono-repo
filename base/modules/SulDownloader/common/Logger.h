// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getSulDownloaderLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSulDownloaderLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getSulDownloaderLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSulDownloaderLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getSulDownloaderLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSulDownloaderLogger(), x)
