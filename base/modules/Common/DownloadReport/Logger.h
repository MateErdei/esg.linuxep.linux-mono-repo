// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getDownloadReportLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getDownloadReportLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getDownloadReportLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getDownloadReportLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getDownloadReportLogger(), x)
