// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IMetadataRescanClientSocket.h"

#include "unixsocket/BaseClient.h"

#include <string>

namespace unixsocket
{
    class MetadataRescanClientSocket : BaseClient, public IMetadataRescanClientSocket
    {
    public:
        explicit MetadataRescanClientSocket(
            std::string socket_path,
            const duration_t& sleepTime = DEFAULT_SLEEP_TIME,
            IStoppableSleeperSharedPtr sleeper = {});

        int connect() override;
        bool sendRequest(const scan_messages::MetadataRescan& metadataRescan) override;
        bool receiveResponse(scan_messages::MetadataRescanResponse& response) override;
        int socketFd() override;
        scan_messages::MetadataRescanResponse rescan(const scan_messages::MetadataRescan& metadataRescan) override;
    };
} // namespace unixsocket
