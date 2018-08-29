/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


namespace Common
{
    namespace Logging
    {
        class ConsoleLoggingSetup
        {
        public:
            ConsoleLoggingSetup();
            ~ConsoleLoggingSetup();
            static void consoleSetupLogging();
        };
    }
}

