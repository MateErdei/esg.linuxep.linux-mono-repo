// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getDownloadReportLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getDownloadReportLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getDownloadReportLogger(), x)   // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getDownloadReportLogger(), x)   // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getDownloadReportLogger(), x) // NOLINT
