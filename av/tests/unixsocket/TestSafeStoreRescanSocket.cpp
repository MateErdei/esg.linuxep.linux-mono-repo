// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "safestore/MockIQuarantineManager.h"
#include "tests/common/Common.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanClient.h"
#include "unixsocket/safeStoreRescanSocket/SafeStoreRescanServerSocket.h"

#include "common/ApplicationPaths.h"

#include <gtest/gtest.h>

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

TEST_F(TestSafeStoreRescanSocket, testSendRescanRequestOnce) // NOLINT
{
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, rescanDatabase()).Times(1);
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    unixsocket::SafeStoreRescanClient client(m_socketPath);
    client.sendRescanRequest();

    sleep(1);
    // destructor will stop the thread
}

TEST_F(TestSafeStoreRescanSocket, testSendRescanRequestTwice) // NOLINT
{
    auto quarantineManager = std::make_shared<MockIQuarantineManager>();
    EXPECT_CALL(*quarantineManager, rescanDatabase()).Times(2);
    unixsocket::SafeStoreRescanServerSocket server(m_socketPath, quarantineManager);
    server.start();

    // connect after we start
    unixsocket::SafeStoreRescanClient client(m_socketPath);
    client.sendRescanRequest();
    client.sendRescanRequest();

    sleep(1);
    // destructor will stop the thread
}

TEST_F(TestSafeStoreRescanSocket, testClientSocketTriesToReconnect) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    unixsocket::SafeStoreRescanClient client(m_socketPath);
    client.sendRescanRequest();

    EXPECT_TRUE(appenderContains("Failed to connect to SafeStore Rescan - retrying after sleep", 9));
    EXPECT_TRUE(appenderContains("Reached total maximum number of connection attempts."));
}