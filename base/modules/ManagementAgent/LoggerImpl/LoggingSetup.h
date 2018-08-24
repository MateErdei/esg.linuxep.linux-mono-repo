/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

namespace ManagementAgent
{
    namespace LoggerImpl
    {
        class LoggingSetup
        {
        public:
            LoggingSetup();

            /// Setting as an overloaded constructor that will be used only in tests.
            /// Hence, most likely removed in production code.
            LoggingSetup(int);

            ~LoggingSetup();
        };
    }

}
