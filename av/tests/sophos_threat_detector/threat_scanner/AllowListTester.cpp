// Copyright 2022 Sophos Limited. All rights reserved.

#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>

#include <tuple>

namespace
{
    class LogSetup
    {
    public:
        LogSetup();
        ~LogSetup();
    };
}


LogSetup::~LogSetup()
{
    log4cplus::Logger::shutdown();
}


LogSetup::LogSetup()
{
    log4cplus::initialize();

    // Log error messages to stderr
    log4cplus::SharedAppenderPtr stderr_appender(new log4cplus::ConsoleAppender(true));
    stderr_appender->setThreshold(log4cplus::TRACE_LOG_LEVEL);
    log4cplus::Logger::getRoot().addAppender(stderr_appender);
}

int main(int argc, char* argv[])
{
    LogSetup logs;



    std::ignore = argc;
    std::ignore = argv;
    return 0;
}