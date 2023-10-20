// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "safestore/ISafeStoreResources.h"
#include "tests/unixsocket/MockIScanningClientSocket.h"
#include "tests/unixsocket/MockRestoreReportingClient.h"
#include "tests/unixsocket/MockMetadataRescanClientSocket.h"

#include <gmock/gmock.h>

#include <string>
#include <memory>

using namespace ::testing;

class MockSafeStoreResources : public safestore::ISafeStoreResources
{
public:
    using duration_t = unixsocket::duration_t;
    using IStoppableSleeperSharedPtr = unixsocket::IStoppableSleeperSharedPtr;
    MockSafeStoreResources()
    {
        ON_CALL(*this, CreateMetadataRescanClientSocket)
            .WillByDefault(
                InvokeWithoutArgs([]() { return std::make_unique<unixsocket::MockMetadataRescanClientSocket>(); }));
        ON_CALL(*this, CreateScanningClientSocket)
            .WillByDefault(InvokeWithoutArgs([]() { return std::make_unique<MockIScanningClientSocket>(); }));
        ON_CALL(*this, CreateRestoreReportingClient)
            .WillByDefault(InvokeWithoutArgs([]() { return std::make_unique<MockRestoreReportingClient>(); }));
    }

    MOCK_METHOD(
        std::unique_ptr<unixsocket::IMetadataRescanClientSocket>,
        CreateMetadataRescanClientSocket,
        (std::string socket_path,
         const duration_t& sleepTime,
         IStoppableSleeperSharedPtr sleeper),
        (override));

    MOCK_METHOD(
        std::unique_ptr<unixsocket::IScanningClientSocket>,
        CreateScanningClientSocket,
        (std::string socket_path,
         const duration_t& sleepTime,
         IStoppableSleeperSharedPtr sleeper),
        (override));

    MOCK_METHOD(
        std::unique_ptr<unixsocket::IRestoreReportingClient>,
        CreateRestoreReportingClient,
        (IStoppableSleeperSharedPtr sleeper),
        (override));
};
