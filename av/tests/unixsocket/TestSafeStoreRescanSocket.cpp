// Copyright 2022 Sophos Limited. All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"


#include "common/ApplicationPaths.h"
#include "safestore/MockIQuarantineManager.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanServerSocket.h"
#include "unixsocket/BaseClient.h"
#include "unixsocket/SocketUtils.h"

#include <gtest/gtest.h>

#include <cassert>
#include <iostream>
#include <string>

namespace
{
    class TestClient : public unixsocket::BaseClient
    {
    public:
        TestClient(
            std::string socket_path,
            const BaseClient::duration_t& sleepTime= std::chrono::seconds{1},
            BaseClient::IStoppableSleeperSharedPtr sleeper={}) :
            BaseClient(std::move(socket_path), sleepTime, std::move(sleeper))
        {
            BaseClient::connectWithRetries("SafeStore Rescan");
        }

        void sendRequest(std::string request)
        {
            assert(m_socket_fd.valid());
            try
            {
                if (!unixsocket::writeLengthAndBuffer(m_socket_fd.get(), request))
                {
                    std::stringstream errMsg;
                    errMsg << "Failed to write rescan request to socket [" << errno << "]";
                    throw std::runtime_error(errMsg.str());
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                std::cerr << "Failed to write to SafeStore Rescan socket. Exception caught: " << e.what();
            }
        }
    };
} // namespace
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
    EXPECT_CALL(*quarantineManager, rescanDatabase()).Times(0);
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    TestClient client(m_socketPath);
    client.sendRequest("0");

    sleep(1);
    // destructor will stop the thread
}
TEST_F(TestSafeStoreRescanSocket, testSendRescanRequestTwice)
{
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    WaitForEvent rescanEvent;
    EXPECT_CALL(*quarantineManager, rescanDatabase()).Times(2)
        .WillOnce(Return())
        .WillOnce(triggerEvent(&rescanEvent));
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    {
        unixsocket::SafeStoreRescanClient client(m_socketPath);
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

    EXPECT_TRUE(appenderContains("Failed to connect to SafeStore Rescan - retrying after sleep", 9));
    EXPECT_TRUE(appenderContains("Reached total maximum number of connection attempts."));
}