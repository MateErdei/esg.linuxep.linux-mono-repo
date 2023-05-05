// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "safestore/ISafeStoreResources.h"
#include "tests/unixsocket/MockMetadataRescanClientSocket.h"

#include <gmock/gmock.h>

using namespace ::testing;

class MockSafeStoreResources : public safestore::ISafeStoreResources
{
public:
    MockSafeStoreResources()
    {
        ON_CALL(*this, CreateMetadataRescanClientSocket)
            .WillByDefault(
                InvokeWithoutArgs([]() { return std::make_unique<unixsocket::MockMetadataRescanClientSocket>(); }));
    }

    MOCK_METHOD(
        std::unique_ptr<unixsocket::IMetadataRescanClientSocket>,
        CreateMetadataRescanClientSocket,
        (std::string socket_path,
         const unixsocket::BaseClient::duration_t& sleepTime,
         unixsocket::BaseClient::IStoppableSleeperSharedPtr sleeper),
        (override));
};
