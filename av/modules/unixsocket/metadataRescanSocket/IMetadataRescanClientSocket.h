// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/MetadataRescan.h"

namespace unixsocket
{
    class IMetadataRescanClientSocket
    {
    public:
        IMetadataRescanClientSocket& operator=(const IMetadataRescanClientSocket&) = delete;
        IMetadataRescanClientSocket(const IMetadataRescanClientSocket&) = delete;
        IMetadataRescanClientSocket() = default;
        virtual ~IMetadataRescanClientSocket() = default;

        virtual int connect() = 0;
        virtual bool sendRequest(const scan_messages::MetadataRescan& metadataRescan) = 0;
        virtual bool receiveResponse(scan_messages::MetadataRescanResponse& response) = 0;
        virtual int socketFd() = 0;
        virtual scan_messages::MetadataRescanResponse rescan(const scan_messages::MetadataRescan& metadataRescan) = 0;
    };
} // namespace unixsocket