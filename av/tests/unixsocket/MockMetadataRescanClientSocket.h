// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "unixsocket/metadataRescanSocket/IMetadataRescanClientSocket.h"

#include <fcntl.h>

#include <gmock/gmock.h>

namespace unixsocket
{
    class MockMetadataRescanClientSocket : public IMetadataRescanClientSocket
    {
    public:
        MockMetadataRescanClientSocket()
        {
            using namespace ::testing;
            m_socketFd.reset(::open("/dev/zero", O_RDONLY));
            ON_CALL(*this, connect).WillByDefault(Return(0));
            ON_CALL(*this, sendRequest).WillByDefault(Return(true));
            ON_CALL(*this, receiveResponse).WillByDefault(Return(true));
            ON_CALL(*this, socketFd).WillByDefault([this]() { return this->m_socketFd.fd(); });
        }

        MOCK_METHOD(int, connect, (), (override));
        MOCK_METHOD(bool, sendRequest, (const scan_messages::MetadataRescan& metadataRescan), (override));
        MOCK_METHOD(bool, receiveResponse, (scan_messages::MetadataRescanResponse & response), (override));
        MOCK_METHOD(int, socketFd, (), (override));
        MOCK_METHOD(
            scan_messages::MetadataRescanResponse,
            rescan,
            (const scan_messages::MetadataRescan& metadataRescan),
            (override));

    private:
        datatypes::AutoFd m_socketFd;
    };
} // namespace unixsocket