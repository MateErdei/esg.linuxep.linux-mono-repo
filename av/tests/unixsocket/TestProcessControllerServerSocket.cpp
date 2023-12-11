// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "common/NotifyPipeSleeper.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/processControllerSocket/ProcessControllerClient.h"
#include "unixsocket/SocketUtils.h"

#include <gtest/gtest.h>

#include <list>

using namespace unixsocket;

namespace fs = sophos_filesystem;

namespace
{
    class TestProcessControllerServerSocket : public UnixSocketMemoryAppenderUsingTests
    {
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            fs::current_path(m_testDir);

            m_socketPath = m_testDir / "process_control_socket";
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;

    public:
        std::string m_socketPath;
    };

    class MockCallback : public IProcessControlMessageCallback
    {
    public:
        void processControlMessage(const scan_messages::E_COMMAND_TYPE& /*command*/) override
        {

        }
    };
}

TEST_F(TestProcessControllerServerSocket, testConstructor)
{
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    EXPECT_NO_THROW(ProcessControllerServerSocket processController(m_socketPath, 0660, callback));
}

TEST_F(TestProcessControllerServerSocket, testSendMessageNoServer)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    constexpr ProcessControllerClientSocket::duration_t sleepTime = std::chrono::microseconds{1};
    ProcessControllerClientSocket processControllerClient(m_socketPath, sleepTime);
    scan_messages::ProcessControlSerialiser processControlRequest(scan_messages::E_SHUTDOWN);
    EXPECT_NO_THROW(processControllerClient.sendProcessControlRequest(processControlRequest));
    EXPECT_FALSE(appenderContains("Failed to write Process Control Request to socket. Exception caught: "));
}

TEST_F(TestProcessControllerServerSocket, testSocketConstruction) // NOLINT
{
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerSocket processController(m_socketPath, 0660, callback);
}


TEST_F(TestProcessControllerServerSocket, testClientSocketTriesToReconnect)
{
    using namespace std::chrono_literals;
    UsingMemoryAppender memoryAppenderHolder(*this);
    ProcessControllerClientSocket processControllerClient(m_socketPath, 1ns);

    EXPECT_TRUE(appenderContains("ProcessControlClient failed to connect to " + m_socketPath + " - retrying upto 10 times with a sleep of 0s", 1));
    EXPECT_TRUE(appenderContains("ProcessControlClient reached the maximum number of connection attempts"));
}

TEST_F(TestProcessControllerServerSocket, TestClientSocketTimeOutInterrupted)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::Threads::NotifyPipe notifyPipe;
    auto notifyPipeSleeper = std::make_unique<common::NotifyPipeSleeper>(notifyPipe);

    std::thread t1([&]() { ProcessControllerClientSocket client{ m_socketPath, std::move(notifyPipeSleeper)}; });

    EXPECT_TRUE(waitForLog("ProcessControlClient failed to connect to " + m_socketPath + " - retrying upto 10 times with a sleep of 1s", 2s));
    notifyPipe.notify();

    t1.join();

    EXPECT_TRUE(appenderContains("ProcessControlClient received stop request while connecting"));
    EXPECT_FALSE(appenderContains("ProcessControlClient reached the maximum number of connection attempts"));
}
