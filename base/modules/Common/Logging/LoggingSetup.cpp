// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "LoggingSetup.h"

using namespace Common::Logging;

void LoggingSetup::applyDefaultPattern(AppenderPtr& appender)
{
    applyPattern(appender, GL_DEFAULT_PATTERN);
}

void LoggingSetup::applyPattern(AppenderPtr& appender, const char* pattern)
{
    std::unique_ptr<log4cplus::Layout> layout(new log4cplus::PatternLayout(pattern));
    appender->setLayout(std::move(layout));
}
