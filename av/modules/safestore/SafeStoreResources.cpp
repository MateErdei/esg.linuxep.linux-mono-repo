// Copyright 2023 Sophos Limited. All rights reserved.

#include "SafeStoreResources.h"

#include "unixsocket/metadataRescanSocket/MetadataRescanClientSocket.h"

namespace safestore
{
    std::unique_ptr<unixsocket::IMetadataRescanClientSocket> SafeStoreResources::CreateMetadataRescanClientSocket(
        std::string socket_path,
        const unixsocket::BaseClient::duration_t& sleepTime,
        unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper)
    {
        return std::make_unique<unixsocket::MetadataRescanClientSocket>(socket_path, sleepTime, sleeper);
    }
} // namespace safestore
