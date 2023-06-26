/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
namespace Common
{
    namespace Logging
    {
        class ConsoleLoggingSetup
        {
        public:
            ConsoleLoggingSetup();
            ConsoleLoggingSetup(const std::string&);
            ~ConsoleLoggingSetup();
            static void consoleSetupLogging();
        };
    } // namespace Logging
} // namespace Common
