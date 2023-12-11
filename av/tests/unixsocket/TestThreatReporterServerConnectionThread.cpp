// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "tests/common/MemoryAppender.h"
#include "unixsocket/threatReporterSocket/ThreatReporterServerConnectionThread.h"

#include "Common/Helpers/MockSysCalls.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fcntl.h>

using namespace unixsocket;
using namespace ::testing;

namespace
{
    class TestThreatReporterServerConnectionThread : public UnixSocketMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        std::shared_ptr<Common::SystemCallWrapper::SystemCallWrapper> m_sysCalls;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
    };

    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(scan_messages::ThreatDetected detection));
    };
}

TEST_F(TestThreatReporterServerConnectionThread, successful_construction)
{
    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);

    EXPECT_NO_THROW(unixsocket::ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_sysCalls));
}

TEST_F(TestThreatReporterServerConnectionThread, isRunning_false_after_construction)
{
    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);

    ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_sysCalls);
    EXPECT_FALSE(connectionThread.isRunning());
}

TEST_F(TestThreatReporterServerConnectionThread, fail_construction_with_bad_fd)
{
    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    EXPECT_THROW(ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_sysCalls), std::runtime_error);
}

TEST_F(TestThreatReporterServerConnectionThread, fail_construction_with_null_factory)
{
    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_THROW(ThreatReporterServerConnectionThread connectionThread(fdHolder, nullptr, m_sysCalls), std::runtime_error);
}

TEST_F(TestThreatReporterServerConnectionThread, stop_while_running)
{
    const std::string expected = "Closing ThreatReporterServerConnectionThread";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_sysCalls);
    EXPECT_FALSE(connectionThread.isRunning());
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // No waiting...
    connectionThread.requestStop();
    connectionThread.join();
    EXPECT_FALSE(connectionThread.isRunning());

    EXPECT_GT(appenderSize(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestThreatReporterServerConnectionThread, eof_while_running)
{
    const std::string expected = "ThreatReporterServerConnectionThread closed: EOF";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestThreatReporterServerConnectionThread, send_zero_length)
{
    const std::string expected = "ThreatReporterServerConnectionThread ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestThreatReporterServerConnectionThread, bad_notify_pipe_fd)
{
    const std::string expected = "Closing ThreatReporterServerConnectionThread, error from notify pipe";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[1].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestThreatReporterServerConnectionThread, bad_socket_fd)
{
    const std::string expected = "Closing ThreatReporterServerConnectionThread, error from socket";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[0].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    ThreatReporterServerConnectionThread connectionThread(fdHolder, mock_callback, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}
