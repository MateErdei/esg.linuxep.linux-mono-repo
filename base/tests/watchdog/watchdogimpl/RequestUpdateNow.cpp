// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/WatchdogRequest/IWatchdogRequest.h"

int main()
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;

    auto request = Common::WatchdogRequest::factory().create();
    request->requestUpdateService();

    return 0;
}
