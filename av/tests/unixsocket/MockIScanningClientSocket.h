// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include <gmock/gmock.h>

class MockIScanningClientSocket : public unixsocket::IScanningClientSocket
{
public:
    MockIScanningClientSocket()
    {
        using namespace ::testing;
        m_socketFd.reset(::open("/dev/zero", O_RDONLY));
        ON_CALL(*this, connect).WillByDefault(Return(0));
        ON_CALL(*this, sendRequest).WillByDefault(Return(true));
        ON_CALL(*this, receiveResponse).WillByDefault(Return(true));
        ON_CALL(*this, socketFd).WillByDefault([this]() { return this->m_socketFd.fd(); });
    }

    MOCK_METHOD(int, connect, ());
    MOCK_METHOD(bool, sendRequest, (scan_messages::ClientScanRequestPtr request));
    MOCK_METHOD(bool, receiveResponse, (scan_messages::ScanResponse & response));
    MOCK_METHOD(int, socketFd, ());

private:
    datatypes::AutoFd m_socketFd;
};