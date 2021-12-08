/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"
#include <unixsocket/processControllerSocket/ProcessControllerServerSocket.h>
#include <unixsocket/processControllerSocket/ProcessControllerClient.h>
#include "unixsocket/SocketUtils.h"

#include <gtest/gtest.h>

#include <list>

#include <unistd.h>

namespace
{
    class TestProcessControllerServerSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        std::string m_socketPath = "/tmp/process_control_socket";
    };
}

TEST_F(TestProcessControllerServerSocket, testConstructor) //NOLINT
{
    EXPECT_NO_THROW(unixsocket::ProcessControllerServerSocket processController(m_socketPath, 0660));
}

TEST_F(TestProcessControllerServerSocket, testSendMessageNoServer)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    unixsocket::ProcessControllerClientSocket processControllerClient(m_socketPath);
    scan_messages::ProcessControlSerialiser processControlRequest;
    EXPECT_NO_THROW(processControllerClient.sendProcessControlRequest(processControlRequest));
    EXPECT_FALSE(appenderContains("Failed to write Process Control Request to socket. Exception caught: "));
}

TEST_F(TestProcessControllerServerSocket, testSocketConstruction) // NOLINT
{
    unixsocket::ProcessControllerServerSocket processController(m_socketPath, 0660);
    EXPECT_FALSE(processController.triggered());
}

TEST_F(TestProcessControllerServerSocket, testTriggerNotified) // NOLINT
{
    unixsocket::ProcessControllerServerSocket processControllerServer(m_socketPath, 0660);
    processControllerServer.start();
    EXPECT_GT(processControllerServer.monitorFd(), -1);

    unixsocket::ProcessControllerClientSocket processControllerClient(m_socketPath);
    scan_messages::ProcessControlSerialiser processControlRequest;
    processControllerClient.sendProcessControlRequest(processControlRequest);

    int retries=0;
    while (!processControllerServer.triggered())
    {
        if (retries > 10)
        {
            FAIL() << "Failed to received shutdown request within 10 attempts";
        }
        sleep(1);
        retries++;
    }

    processControllerServer.requestStop();
    processControllerServer.join();

    EXPECT_TRUE(processControllerServer.triggered());
}
