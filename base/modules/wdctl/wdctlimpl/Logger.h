// Copyright 2018-2024 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getWdctlLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getWdctlLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getWdctlLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getWdctlLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getWdctlLogger(), x)
// wdctl depends on the assumption that it logs errors to both the log file and stderr. The logger class no longer
// forwards errors to stderr, so they need to be put there in addition.
#define LOGERROR(x)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        std::cerr << x << std::endl;                                                                                   \
        LOG4CPLUS_ERROR(getWdctlLogger(), x);                                                                          \
    } while (0)
