// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/RestoreReport.h"
#include "unixsocket/BaseClient.h"

#include <string>

namespace unixsocket
{
    class RestoreReportingClient : public unixsocket::BaseClient
    {
    public:
        explicit RestoreReportingClient(std::shared_ptr<common::StoppableSleeper> sleeper);

        void sendRestoreReport(const scan_messages::RestoreReport& restoreReport);
    };
} // namespace unixsocket
