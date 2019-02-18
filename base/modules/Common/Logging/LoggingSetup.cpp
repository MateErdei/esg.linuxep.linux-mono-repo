/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "LoggingSetup.h"

using namespace Common::Logging;

// std::string pattern("%d{%m/%d/%y  %H:%M:%S}  - %m [%l]%n");
const char* LoggingSetup::GL_DEFAULT_PATTERN = "%-7r [%d{%Y-%m-%dT%H:%M:%S.%Q}] %5p [%10.10t] %c <> %m%n";

const char* LoggingSetup::GL_CONSOLE_PATTERN = "[%d{%H:%M:%S}] %m%n";

void LoggingSetup::applyDefaultPattern(AppenderPtr& appender)
{
    applyPattern(appender, GL_DEFAULT_PATTERN);
}

void LoggingSetup::applyPattern(AppenderPtr& appender, const char* pattern)
{
    std::unique_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(pattern)); // NOLINT
    appender->setLayout(std::move(layout));
}
