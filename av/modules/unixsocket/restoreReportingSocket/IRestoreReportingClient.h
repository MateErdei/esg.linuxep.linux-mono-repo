// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/RestoreReport.h"

namespace unixsocket
{
    class IRestoreReportingClient
    {
    public:
        virtual ~IRestoreReportingClient() = default;

        virtual void sendRestoreReport(const scan_messages::RestoreReport& restoreReport) = 0;
    };
} // namespace unixsocket
