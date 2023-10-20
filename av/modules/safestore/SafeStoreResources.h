// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISafeStoreResources.h"

namespace safestore
{
    class SafeStoreResources : public ISafeStoreResources
    {
    public:
        std::unique_ptr<unixsocket::IMetadataRescanClientSocket> CreateMetadataRescanClientSocket(
            std::string socket_path,
            const duration_t& sleepTime,
            SleeperSharedPtr sleeper) override;

        std::unique_ptr<unixsocket::IScanningClientSocket> CreateScanningClientSocket(
            std::string socket_path,
            const duration_t& sleepTime,
            SleeperSharedPtr sleeper) override;

        std::unique_ptr<unixsocket::IRestoreReportingClient> CreateRestoreReportingClient(
                SleeperSharedPtr sleeper) override;
    };
} // namespace safestore
