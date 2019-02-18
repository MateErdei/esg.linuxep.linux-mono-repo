/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <iostream>

log4cplus::Logger& getSulDownloaderLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSulDownloaderLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSulDownloaderLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getSulDownloaderLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getSulDownloaderLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSulDownloaderLogger(), x)  // NOLINT
