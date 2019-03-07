/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <memory>

namespace Tests
{
    class TestConsoleLoggingSetup
    {
    public:
        TestConsoleLoggingSetup();
        ~TestConsoleLoggingSetup();
        static void consoleSetupLogging();
    };
    using TestConsoleLoggingSetupPtr = std::unique_ptr<TestConsoleLoggingSetup>;
} // namespace TestLogging
