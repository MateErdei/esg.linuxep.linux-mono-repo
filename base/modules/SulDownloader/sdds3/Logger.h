// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getSulDownloaderSdds3Logger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSulDownloaderSdds3Logger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getSulDownloaderSdds3Logger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getSulDownloaderSdds3Logger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getSulDownloaderSdds3Logger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getSulDownloaderSdds3Logger(), x)
