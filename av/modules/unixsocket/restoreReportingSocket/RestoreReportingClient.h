// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IRestoreReportingClient.h"

#include "unixsocket/BaseClient.h"

namespace unixsocket
{
    class RestoreReportingClient : public BaseClient, public IRestoreReportingClient
    {
    public:
        explicit RestoreReportingClient(std::shared_ptr<common::StoppableSleeper> sleeper);

        void sendRestoreReport(const scan_messages::RestoreReport& restoreReport) override;
    };
} // namespace unixsocket
