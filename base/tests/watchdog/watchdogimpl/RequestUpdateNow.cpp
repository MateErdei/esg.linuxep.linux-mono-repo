// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logging/ConsoleLoggingSetup.h"

#include "watchdog/watchdogimpl/WatchdogServiceLine.h"

int main()
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;

    auto request = watchdog::watchdogimpl::factory().create();
    request->requestUpdateService();

    return 0;
}
