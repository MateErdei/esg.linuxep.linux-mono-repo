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
            const unixsocket::BaseClient::duration_t& sleepTime,
            unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper) override;

        std::unique_ptr<unixsocket::IScanningClientSocket> CreateScanningClientSocket(
            std::string socket_path,
            const unixsocket::BaseClient::duration_t& sleepTime,
            unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper) override;

        std::unique_ptr<unixsocket::IRestoreReportingClient> CreateRestoreReportingClient(
            unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper) override;
    };
} // namespace safestore