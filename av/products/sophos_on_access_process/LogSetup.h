/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/LoggingSetup.h>
#include <Common/Logging/SophosLoggerMacros.h>

#include <log4cplus/logger.h>

log4cplus::Logger& getSophosOnAccessLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getSophosOnAccessLogger(), x)    // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getSophosOnAccessLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getSophosOnAccessLogger(), x)    // NOLINT

class LogSetup
{
public:
    LogSetup();
    ~LogSetup();
};
