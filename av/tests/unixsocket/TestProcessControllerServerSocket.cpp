/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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
}

TEST_F(TestProcessControllerServerSocket, testConstructor) //NOLINT
{
    EXPECT_NO_THROW(unixsocket::ProcessControllerServerSocket processController(m_socketPath, 0660));
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
    unixsocket::ProcessControllerServerSocket processController(m_socketPath, 0660);
    EXPECT_FALSE(processController.triggeredShutdown());
}

TEST_F(TestProcessControllerServerSocket, testTriggerNotified) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    {
        unixsocket::ProcessControllerServerSocket processControllerServer(m_socketPath, 0660);
        processControllerServer.start();
        EXPECT_GT(processControllerServer.monitorShutdownFd(), -1);

        unixsocket::ProcessControllerClientSocket processControllerClient(m_socketPath);
        scan_messages::ProcessControlSerialiser processControlRequest(scan_messages::E_SHUTDOWN);
        processControllerClient.sendProcessControlRequest(processControlRequest);

        int retries = 0;
        while (!processControllerServer.triggeredShutdown()) {
            if (retries > 10) {
                FAIL() << "Failed to received shutdown request within 10 attempts";
            }
            sleep(1);
            retries++;
        }
        EXPECT_TRUE(processControllerServer.triggeredShutdown());
    }

    EXPECT_TRUE(appenderContains("Process Controller Server starting listening on socket"));
    EXPECT_TRUE(appenderContains("Process Controller Server accepting connection"));
    EXPECT_TRUE(appenderContains("Process Controller Server thread got connection"));
    EXPECT_TRUE(appenderContains("Closing Process Controller Server socket"));
    //destructor will stop the thread
}
