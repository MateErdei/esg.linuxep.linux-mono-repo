/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

#include <string>

log4cplus::Logger& getNamedScanRunnerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getNamedScanRunnerLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getNamedScanRunnerLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getNamedScanRunnerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getNamedScanRunnerLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getNamedScanRunnerLogger(), x)  // NOLINT

class Logger
{
public:
    explicit Logger(const std::string& scanName, bool isCommandLine=false);
    ~Logger();
};



