// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"
#include "TestClient.h"

#include "common/ApplicationPaths.h"
#include "safestore/MockIQuarantineManager.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanServerSocket.h"
#include "unixsocket/BaseClient.h"
#include "unixsocket/SocketUtils.h"

#include <gtest/gtest.h>


#include <string>

using namespace testing;
namespace fs = sophos_filesystem;

namespace
{
    class TestSafeStoreRescanSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setupFakeSophosThreatDetectorConfig();
            m_socketPath = Plugin::getSafeStoreRescanSocketPath();
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        std::string m_socketPath;
    };
} // namespace

TEST_F(TestSafeStoreRescanSocket, testSendRescanRequestOnce)
{
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    WaitForEvent rescanEvent;
    EXPECT_CALL(*quarantineManager, rescanDatabase()).WillOnce(triggerEvent(&rescanEvent));
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    {
        unixsocket::SafeStoreRescanClient client(m_socketPath);
        client.sendRescanRequest();
    }

    rescanEvent.waitDefault();
    // destructor will stop the thread
}

TEST_F(TestSafeStoreRescanSocket, testInvalidRescanRequest)
{
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, rescanDatabase()).Times(1);
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    {
        TestClient client(m_socketPath);
        client.sendRequest("0");
        client.sendRequest("1");
    }

    sleep(1);
    // destructor will stop the thread
}

TEST_F(TestSafeStoreRescanSocket, testLongRescanRequest)
{
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, rescanDatabase()).Times(1);
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    {
        TestClient client(m_socketPath);
        std::string request = "1";
        for (int length = 0; length < 10000; length++)
        {
            request = request + "s";
        }
        client.sendRequest(request);
    }

    sleep(1);
    // destructor will stop the thread
}
TEST_F(TestSafeStoreRescanSocket, testSendRescanRequestMultipleTimes)
{
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    WaitForEvent rescanEvent;
    EXPECT_CALL(*quarantineManager, rescanDatabase()).Times(10)
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(triggerEvent(&rescanEvent));
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    {
        unixsocket::SafeStoreRescanClient client(m_socketPath);
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();
        client.sendRescanRequest();

    }

    rescanEvent.waitDefault();
    // destructor will stop the thread
}

TEST_F(TestSafeStoreRescanSocket, testClientSocketTriesToReconnect)
{
    using namespace std::chrono_literals;
    UsingMemoryAppender memoryAppenderHolder(*this);
    unixsocket::SafeStoreRescanClient client(m_socketPath, 1ns);
    client.sendRescanRequest();

    EXPECT_TRUE(appenderContains("Failed to connect to SafeStore Rescan - retrying upto 10 times with a sleep of 0s", 1));
    EXPECT_TRUE(appenderContains("Reached the maximum number of attempts connecting to SafeStore Rescan"));
}