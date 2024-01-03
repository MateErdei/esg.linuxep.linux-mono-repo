// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include <string>
namespace Common::Logging
{
    class ConsoleLoggingSetup
    {
    public:
        ConsoleLoggingSetup();
        ConsoleLoggingSetup(const std::string&);
        ~ConsoleLoggingSetup();
        static void consoleSetupLogging();
        static void loggingShutdown();
    };
} // namespace Common::Logging
