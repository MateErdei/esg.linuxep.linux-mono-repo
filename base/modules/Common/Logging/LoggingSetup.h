/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/appender.h>

namespace Common
{
    namespace Logging
    {
        using AppenderPtr = log4cplus::SharedAppenderPtr;

        class LoggingSetup
        {
        public:
            /**
             * Log pattern for standard log files
             */
            static const char* GL_DEFAULT_PATTERN;
            /**
             * Simplified pattern for release Console output.
             */
            static const char* GL_CONSOLE_PATTERN;

            static void applyDefaultPattern(AppenderPtr& appender);

            static void applyPattern(AppenderPtr& appender, const char* pattern);
        };
    } // namespace Logging
} // namespace Common
