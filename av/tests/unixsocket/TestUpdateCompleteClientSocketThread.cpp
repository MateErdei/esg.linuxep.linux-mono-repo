// Copyright 2022, Sophos Limited.  All rights reserved.

#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"
#include "unixsocket/updateCompleteSocket/UpdateCompleteServerSocket.h"

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"

#include "tests/common/WaitForEvent.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <thread>

using namespace unixsocket::updateCompleteSocket;
using namespace ::testing;

namespace fs = sophos_filesystem;

namespace
{
    class MockIUpdateCompleteCallback : public IUpdateCompleteCallback
    {
    public:
        MOCK_METHOD(void, updateComplete, (), (override));
    };

    class TestUpdateCompleteClientSocketThread : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            fs::current_path(m_testDir);
            m_socketPath = m_testDir / "socket";
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
        fs::path m_socketPath;
    };
}

TEST_F(TestUpdateCompleteClientSocketThread, constructionDoesNotCallback)
{
    auto callback = std::make_shared<StrictMock<MockIUpdateCompleteCallback>>();
    UpdateCompleteClientSocketThread foo(m_socketPath, callback);
}

TEST_F(TestUpdateCompleteClientSocketThread, notificationPassedToClient)
{
    WaitForEvent callbackGuard;
    auto callback = std::make_shared<StrictMock<MockIUpdateCompleteCallback>>();
    EXPECT_CALL(*callback, updateComplete()).WillOnce(
        InvokeWithoutArgs(&callbackGuard, &WaitForEvent::onEventNoArgs));

    // Start server
    UpdateCompleteServerSocket server(m_socketPath, 0700);
    server.start();

    // Start client
    UpdateCompleteClientSocketThread client(m_socketPath, callback);
    client.start();

    int count = 20;
    while (count > 0 && (server.clientCount() == 0 || !client.connected()))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        count--;
    }

    EXPECT_TRUE(client.connected());
    EXPECT_EQ(server.clientCount(), 1);

    server.publishUpdateComplete();

    // wait for callback
    callbackGuard.wait(5*1000);

    client.tryStop();
    server.tryStop();

    client.join();
    server.join();
}

TEST_F(TestUpdateCompleteClientSocketThread, clientConnectsIfStartedFirst)
{
    WaitForEvent callbackGuard;
    auto callback = std::make_shared<StrictMock<MockIUpdateCompleteCallback>>();
    EXPECT_CALL(*callback, updateComplete()).WillOnce(
        InvokeWithoutArgs(&callbackGuard, &WaitForEvent::onEventNoArgs));

    // Start client
    UpdateCompleteClientSocketThread client(m_socketPath, callback);
    client.start();

    // Start server
    UpdateCompleteServerSocket server(m_socketPath, 0700);
    server.start();

    int count = 20;
    while (count > 0 && (server.clientCount() == 0 || !client.connected()))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        count--;
    }

    EXPECT_TRUE(client.connected());
    EXPECT_EQ(server.clientCount(), 1);

    server.publishUpdateComplete();

    // wait for callback
    callbackGuard.wait(5*1000);

    client.tryStop();
    server.tryStop();

    client.join();
    server.join();
}
