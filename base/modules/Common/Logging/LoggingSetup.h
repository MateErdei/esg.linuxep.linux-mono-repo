// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include <log4cplus/appender.h>

namespace Common::Logging
{
    using AppenderPtr = log4cplus::SharedAppenderPtr;

    class LoggingSetup
    {
    public:
        /**
         * Log pattern for standard log files
         */
        static inline constexpr const char* const GL_DEFAULT_PATTERN =
            "%-7r [%d{%Y-%m-%dT%H:%M:%S.%q}] %7p [%10.10t] %c <> %m%n";
        /**
         * Simplified pattern for release Console output.
         */
        static inline constexpr const char* const GL_CONSOLE_PATTERN = "[%d{%H:%M:%S}] %p %m%n";

        static void applyDefaultPattern(AppenderPtr& appender);

        static void applyPattern(AppenderPtr& appender, const char* pattern);
    };
} // namespace Common::Logging
