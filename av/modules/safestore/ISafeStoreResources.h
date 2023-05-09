// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "unixsocket/BaseClient.h"
#include "unixsocket/metadataRescanSocket/IMetadataRescanClientSocket.h"
#include "unixsocket/restoreReportingSocket/IRestoreReportingClient.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

namespace safestore
{
    class ISafeStoreResources
    {
    public:
        virtual ~ISafeStoreResources() = default;

        virtual std::unique_ptr<unixsocket::IMetadataRescanClientSocket> CreateMetadataRescanClientSocket(
            std::string socket_path,
            const unixsocket::BaseClient::duration_t& sleepTime = unixsocket::BaseClient::DEFAULT_SLEEP_TIME,
            unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper = {}) = 0;

        virtual std::unique_ptr<unixsocket::IScanningClientSocket> CreateScanningClientSocket(
            std::string socket_path,
            const unixsocket::BaseClient::duration_t& sleepTime = unixsocket::BaseClient::DEFAULT_SLEEP_TIME,
            unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper = {}) = 0;

        virtual std::unique_ptr<unixsocket::IRestoreReportingClient> CreateRestoreReportingClient(
            unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper = {}) = 0;
    };
} // namespace safestore