// Copyright 2023 Sophos Limited. All rights reserved.

#include "SafeStoreResources.h"

#include "unixsocket/metadataRescanSocket/MetadataRescanClientSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/restoreReportingSocket/RestoreReportingClient.h"

namespace safestore
{
    std::unique_ptr<unixsocket::IMetadataRescanClientSocket> SafeStoreResources::CreateMetadataRescanClientSocket(
        std::string socket_path,
        const unixsocket::BaseClient::duration_t& sleepTime,
        unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper)
    {
        return std::make_unique<unixsocket::MetadataRescanClientSocket>(socket_path, sleepTime, sleeper);
    }

    std::unique_ptr<unixsocket::IScanningClientSocket> SafeStoreResources::CreateScanningClientSocket(
        std::string socket_path,
        const unixsocket::BaseClient::duration_t& sleepTime,
        unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper)
    {
        return std::make_unique<unixsocket::ScanningClientSocket>(socket_path, sleepTime, std::move(sleeper));
    }

    std::unique_ptr<unixsocket::IRestoreReportingClient> SafeStoreResources::CreateRestoreReportingClient(
        unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper)
    {
        return std::make_unique<unixsocket::RestoreReportingClient>(std::move(sleeper));
    }
} // namespace safestore
