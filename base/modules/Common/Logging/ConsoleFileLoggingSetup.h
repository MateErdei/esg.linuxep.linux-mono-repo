/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
namespace Common
{
    namespace Logging
    {
        class ConsoleFileLoggingSetup
        {
        public:
            explicit ConsoleFileLoggingSetup(const std::string& logbase, bool lowpriv = false);
            ~ConsoleFileLoggingSetup();
            static void setupConsoleFileLogging(const std::string& logbase, bool lowpriv = false);
            static void setupConsoleFileLoggingWithPath(const std::string& logfilepath);
        };
    } // namespace Logging
} // namespace Common

