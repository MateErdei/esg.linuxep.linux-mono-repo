// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"
#include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"
#include <unixsocket/processControllerSocket/ProcessControllerServerSocket.h>
#include <unixsocket/processControllerSocket/ProcessControllerClient.h>
#include "unixsocket/SocketUtils.h"

#include <gtest/gtest.h>

#include <list>

#include <unistd.h>

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

TEST_F(TestProcessControllerServerSocket, testConstructor) //NOLINT
{
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    EXPECT_NO_THROW(unixsocket::ProcessControllerServerSocket processController(m_socketPath, 0660, callback));
}

TEST_F(TestProcessControllerServerSocket, testSendMessageNoServer)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const struct timespec sleepTime {0,1'000'000};
    unixsocket::ProcessControllerClientSocket processControllerClient(m_socketPath, sleepTime);
    scan_messages::ProcessControlSerialiser processControlRequest(scan_messages::E_SHUTDOWN);
    EXPECT_NO_THROW(processControllerClient.sendProcessControlRequest(processControlRequest));
    EXPECT_FALSE(appenderContains("Failed to write Process Control Request to socket. Exception caught: "));
}

TEST_F(TestProcessControllerServerSocket, testSocketConstruction) // NOLINT
{
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    unixsocket::ProcessControllerServerSocket processController(m_socketPath, 0660, callback);
}