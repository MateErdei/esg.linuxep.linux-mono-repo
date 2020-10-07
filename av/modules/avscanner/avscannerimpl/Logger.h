/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Logging/SophosLoggerMacros.h>

#include <string>
#include <pluginapi/include/Common/Logging/LoggerConfig.h>

log4cplus::Logger& getNamedScanRunnerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getNamedScanRunnerLogger(), x)  // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getNamedScanRunnerLogger(), x)    // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getNamedScanRunnerLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getNamedScanRunnerLogger(), x)    // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getNamedScanRunnerLogger(), x)  // NOLINT

class Logger
{
private:
    void setupFileLoggingWithPath(std::string logfilepath);

public:
    explicit Logger(const std::string& scanName, log4cplus::LogLevel logLevel=log4cplus::NOT_SET_LOG_LEVEL, bool isCommandLine=false);
    void applyCommandLineLevel(const log4cplus::LogLevel& CLSlogLevel);

    ~Logger();
};



