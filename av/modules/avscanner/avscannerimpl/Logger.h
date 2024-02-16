// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

#include <string>
#include "Common/Logging/LoggerConfig.h"

log4cplus::Logger& getNamedScanRunnerLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getNamedScanRunnerLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getNamedScanRunnerLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getNamedScanRunnerLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getNamedScanRunnerLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getNamedScanRunnerLogger(), x)

class Logger
{
private:
    void setupFileLoggingWithPath(const std::string& logfilepath);
    void applyCommandLineLevel(const log4cplus::LogLevel& CLSlogLevel);

public:
    explicit Logger(const std::string& scanName, log4cplus::LogLevel logLevel=log4cplus::NOT_SET_LOG_LEVEL, bool isCommandLine=false);

    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};



