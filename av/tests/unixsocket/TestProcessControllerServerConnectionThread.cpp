/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "unixsocket/processControllerSocket/ProcessControllerServerConnectionThread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fcntl.h>

using namespace unixsocket;
using namespace ::testing;

namespace
{
    class TestProcessControllerServerConnectionThread : public UnixSocketMemoryAppenderUsingTests
    {
    };

    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(const scan_messages::ServerThreatDetected& detection));
    };
}

TEST_F(TestProcessControllerServerConnectionThread, successful_construction) //NOLINT
{
    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);

    EXPECT_NO_THROW(unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, shutdownPipe));
}

TEST_F(TestProcessControllerServerConnectionThread, isRunning_false_after_construction) //NOLINT
{
    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);

    unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, shutdownPipe);
    EXPECT_FALSE(connectionThread.isRunning());
}

TEST_F(TestProcessControllerServerConnectionThread, fail_construction_with_bad_fd) //NOLINT
{
    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    EXPECT_THROW(ProcessControllerServerConnectionThread connectionThread(fdHolder, shutdownPipe), std::runtime_error);
}

TEST_F(TestProcessControllerServerConnectionThread, fail_construction_with_null_factory) //NOLINT
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_THROW(ProcessControllerServerConnectionThread connectionThread(fdHolder, nullptr), std::runtime_error);
}
//
//TEST_F(TestProcessControllerServerConnectionThread, stop_while_running) //NOLINT
//{
//    const std::string expected = "Closing scanning socket thread";
//    UsingMemoryAppender memoryAppenderHolder(*this);
//
//    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
//    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
//    ASSERT_GE(fdHolder.get(), 0);
//    ProcessControllerServerConnectionThread connectionThread(fdHolder, mock_callback);
//    EXPECT_FALSE(connectionThread.isRunning());
//    connectionThread.start();
//    EXPECT_TRUE(connectionThread.isRunning());
//    // No waiting...
//    connectionThread.requestStop();
//    connectionThread.join();
//    EXPECT_FALSE(connectionThread.isRunning());
//
//    EXPECT_GT(appenderSize(), 0);
//    EXPECT_TRUE(appenderContains(expected));
//}
//
//TEST_F(TestProcessControllerServerConnectionThread, eof_while_running) //NOLINT
//{
//    const std::string expected = "ThreatReporter Connection closed: EOF";
//    UsingMemoryAppender memoryAppenderHolder(*this);
//
//    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
//    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
//    ASSERT_GE(fdHolder.get(), 0);
//    ProcessControllerServerConnectionThread connectionThread(fdHolder, mock_callback);
//    connectionThread.start();
//    waitForLog(expected);
//    connectionThread.requestStop();
//    connectionThread.join();
//
//    EXPECT_GT(m_memoryAppender->size(), 0);
//    EXPECT_TRUE(appenderContains(expected));
//}
//
//TEST_F(TestProcessControllerServerConnectionThread, send_zero_length) //NOLINT
//{
//    const std::string expected = "Ignoring length of zero / No new messages";
//    UsingMemoryAppender memoryAppenderHolder(*this);
//
//    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
//    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
//    ASSERT_GE(fdHolder.get(), 0);
//    ProcessControllerServerConnectionThread connectionThread(fdHolder, mock_callback);
//    connectionThread.start();
//    waitForLog(expected);
//    connectionThread.requestStop();
//    connectionThread.join();
//
//    EXPECT_GT(m_memoryAppender->size(), 0);
//    EXPECT_TRUE(appenderContains(expected));
//}
//
//TEST_F(TestProcessControllerServerConnectionThread, closed_fd) //NOLINT
//{
//    const std::string expected = "Closing socket, error: 9";
//    UsingMemoryAppender memoryAppenderHolder(*this);
//
//    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
//    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
//    ASSERT_GE(fdHolder.get(), 0);
//    int fd = fdHolder.get();
//    ProcessControllerServerConnectionThread connectionThread(fdHolder, mock_callback);
//    ::close(fd); // fd in connection Thread now broken
//    connectionThread.start();
//    waitForLog(expected);
//    connectionThread.requestStop();
//    connectionThread.join();
//
//    EXPECT_GT(m_memoryAppender->size(), 0);
//    EXPECT_TRUE(appenderContains(expected));
//}
