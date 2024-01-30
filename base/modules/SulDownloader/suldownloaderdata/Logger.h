// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getSulDownloaderDataLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSulDownloaderDataLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getSulDownloaderDataLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSulDownloaderDataLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getSulDownloaderDataLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSulDownloaderDataLogger(), x)
