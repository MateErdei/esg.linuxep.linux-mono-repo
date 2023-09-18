// Copyright 2021-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "UnixSocketMemoryAppenderUsingTests.h"
#include "common/WaitForEvent.h"

#include "datatypes/sophos_filesystem.h"
#include "tests/common/MemoryAppender.h"
#include "tests/common/TestFile.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerConnectionThread.h"

#include "Common/Helpers/MockSysCalls.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <gtest/gtest.h>
#include <scan_messages/ProcessControlSerialiser.h>
#include <scan_messages/ClientScanRequest.h> // for invalid_request_type test

#include <memory>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fs = sophos_filesystem;

using namespace unixsocket;
using namespace ::testing;

namespace
{
    class TestProcessControllerServerConnectionThread : public UnixSocketMemoryAppenderUsingTests
    {
    protected:
        TestProcessControllerServerConnectionThread() : memoryAppenderHolder(*this)
        {
        }

        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            fs::current_path(m_testDir);

            m_sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        UsingMemoryAppender memoryAppenderHolder;
        fs::path m_testDir;
        std::shared_ptr<Common::SystemCallWrapper::SystemCallWrapper> m_sysCalls;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
    };

    class MockCallback : public IProcessControlMessageCallback
    {
    public:
        MOCK_METHOD(void, processControlMessage, (const scan_messages::E_COMMAND_TYPE&), (override));

        MockCallback()
        {
            ON_CALL(*this, processControlMessage).WillByDefault(Return());
        }
    };
}

TEST_F(TestProcessControllerServerConnectionThread, successful_construction)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    EXPECT_NO_THROW(unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_sysCalls));
}

TEST_F(TestProcessControllerServerConnectionThread, isRunning_false_after_construction)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_sysCalls);
    EXPECT_FALSE(connectionThread.isRunning());
}

TEST_F(TestProcessControllerServerConnectionThread, fail_construction_with_bad_fd)
{
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    EXPECT_THROW(unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_sysCalls),
                 std::runtime_error);
}

TEST_F(TestProcessControllerServerConnectionThread, stop_while_running)
{
    const std::string expected = "Closing ProcessControllerServerConnectionThread";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_sysCalls);
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

TEST_F(TestProcessControllerServerConnectionThread, eof_while_running)
{
    const std::string expected = "ProcessControllerServerConnectionThread closed: EOF";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, send_zero_length)
{
    const unsigned char zeroLengthMessage[] = { 0x00 };
    const std::string expected = "ProcessControllerServerConnectionThread ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);

    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.start();

    ::send(clientFd.get(), zeroLengthMessage, sizeof(zeroLengthMessage), MSG_NOSIGNAL);
    EXPECT_TRUE(waitForLog(expected));

    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, send_zero_length_twice_logs_once)
{
    const unsigned char zeroLengthMessage[] = { 0x00 };
    const std::string expected = "ProcessControllerServerConnectionThread ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);

    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.start();

    ::send(clientFd.get(), zeroLengthMessage, sizeof(zeroLengthMessage), MSG_NOSIGNAL);
    EXPECT_TRUE(waitForLog(expected, 100ms));

    clearMemoryAppender();
    ::send(clientFd.get(), zeroLengthMessage, sizeof(zeroLengthMessage), MSG_NOSIGNAL);
    EXPECT_FALSE(waitForLog(expected, 200ms));

    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, send_zero_length_twice_logs_again_after_valid_message)
{
    const unsigned char zeroLengthMessage[] = { 0x00 };
    const std::string expected = "ProcessControllerServerConnectionThread ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);

    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.start();

    ::send(clientFd.get(), zeroLengthMessage, sizeof(zeroLengthMessage), MSG_NOSIGNAL);
    EXPECT_TRUE(waitForLog(expected, 100ms));

    EXPECT_CALL(*callback, processControlMessage(_));
    scan_messages::ProcessControlSerialiser processControlRequest(scan_messages::E_RELOAD);
    unixsocket::writeLengthAndBuffer(clientFd.get(), processControlRequest.serialise());

    clearMemoryAppender();
    ::send(clientFd.get(), zeroLengthMessage, sizeof(zeroLengthMessage), MSG_NOSIGNAL);
    EXPECT_TRUE(waitForLog(expected, 100ms));

    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, bad_notify_pipe_fd)
{
    const std::string expected = "Closing ProcessControllerServerConnectionThread, error from notify pipe";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();

    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLERR));

    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, bad_socket_fd)
{
    const std::string expected = "Closing ProcessControllerServerConnectionThread, error from socket";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();

    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(0, POLLERR));

    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();
}


TEST_F(TestProcessControllerServerConnectionThread, ignore_EINTR)
{
    const std::string expected = "Closing ProcessControllerServerConnectionThread\n";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();

    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(SetErrnoAndReturn(EINTR, -1))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));

    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, ppoll_fails_with_EINVAL)
{
    const std::string expected = "ProcessControllerServerConnectionThread got error from ppoll: Invalid argument";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();

    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr))
        .WillOnce(SetErrnoAndReturn(EINVAL, -1));

    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, over_max_length)
{
    const std::string expected = "Aborting ProcessControllerServerConnectionThread: failed to read length";

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLength(clientFd.get(), 0x1000080);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, max_length)
{
    const std::string expected = "ProcessControllerServerConnectionThread aborting socket connection: failed to read entire message";

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.readTimeout_ = std::chrono::milliseconds{10};
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // length is limited to ~16MB
    unixsocket::writeLength(clientFd.get(), 0x100007f);
    ::close(clientFd.get());
    EXPECT_TRUE(waitForLog(expected, 500ms));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, corrupt_request)
{
    const std::string expected = "Terminated ProcessControllerServerConnectionThread with serialisation exception: ";

    const std::string request = { 0x01, 0x00 };

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd.get(), request);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, valid_request)
{
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);

    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());

    WaitForEvent gotEvent;
    EXPECT_CALL(*callback, processControlMessage(scan_messages::E_SHUTDOWN))
        .WillOnce(triggerEvent(&gotEvent));
    scan_messages::ProcessControlSerialiser processControlRequest(scan_messages::E_SHUTDOWN);
    unixsocket::writeLengthAndBuffer(clientFd.get(), processControlRequest.serialise());
    gotEvent.wait(500ms);

    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestProcessControllerServerConnectionThread, invalid_request_type)
{
    // capnp doesn't check the message type, so we're just testing that it doesn't crash
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);

    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());

    WaitForEvent gotEvent;
    EXPECT_CALL(*callback, processControlMessage(_))
        .WillOnce(triggerEvent(&gotEvent));
    scan_messages::ClientScanRequest clientScanRequest;
    unixsocket::writeLengthAndBuffer(clientFd.get(), clientScanRequest.serialise());
    gotEvent.wait(500ms);

    connectionThread.requestStop();
    connectionThread.join();
}
