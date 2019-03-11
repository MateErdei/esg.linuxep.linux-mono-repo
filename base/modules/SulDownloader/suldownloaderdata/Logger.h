/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/Logging/SophosLoggerMacros.h>

log4cplus::Logger& getSulDownloaderDataLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSulDownloaderDataLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSulDownloaderDataLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSulDownloaderDataLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSulDownloaderDataLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSulDownloaderDataLogger(), x)  // NOLINT
