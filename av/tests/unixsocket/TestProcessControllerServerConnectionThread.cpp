/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"
#include "tests/common/MemoryAppender.h"
#include "tests/common/TestFile.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerConnectionThread.h"

#include <gtest/gtest.h>
#include <scan_messages/ProcessControlSerialiser.h>

#include <memory>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ProcessControl.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

namespace fs = sophos_filesystem;

using namespace unixsocket;
using namespace ::testing;

namespace
{
    class MockProcessControlSerialiser : public scan_messages::ProcessControlSerialiser
    {
    public:
        MockProcessControlSerialiser(): scan_messages::ProcessControlSerialiser()
        {
            m_intCommandType = m_commandType;
        }

        void setCommandTypeInt(int commandType)
        {
            m_intCommandType = commandType;
        }

        std::string serialise() const
        {
            ::capnp::MallocMessageBuilder message;
            Sophos::ssplav::ProcessControl::Builder processControlBuilder =
                message.initRoot<Sophos::ssplav::ProcessControl>();

            processControlBuilder.setCommandType(m_intCommandType);

            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        int m_intCommandType = 1;
    };

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
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        UsingMemoryAppender memoryAppenderHolder;
        fs::path m_testDir;
    };

    class TestProcessControllerServerConnectionThreadWithSocketPair : public TestProcessControllerServerConnectionThread
    {
    protected:
        void SetUp() override
        {
            TestProcessControllerServerConnectionThread::SetUp();

            int socket_fds[2];
            int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
            ASSERT_EQ(ret, 0);
            m_serverFd.reset(socket_fds[0]);
            m_clientFd.reset(socket_fds[1]);
            ASSERT_GE(m_serverFd.get(), 0);
            ASSERT_GE(m_clientFd.get(), 0);
        }

        datatypes::AutoFd m_serverFd;
        datatypes::AutoFd m_clientFd;
    };

    class TestProcessControllerServerConnectionThreadWithSocketConnection :
        public TestProcessControllerServerConnectionThreadWithSocketPair
    {
    protected:
        void SetUp() override
        {
            TestProcessControllerServerConnectionThreadWithSocketPair::SetUp();

            auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
            m_connectionThread = std::make_shared<ProcessControllerServerConnectionThread>(m_serverFd, shutdownPipe);
        }

        std::shared_ptr<ProcessControllerServerConnectionThread> m_connectionThread;
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
    ASSERT_THROW(ProcessControllerServerConnectionThread(fdHolder, nullptr), std::runtime_error);
}

TEST_F(TestProcessControllerServerConnectionThread, stop_while_running) //NOLINT
{
    const std::string expected = "Closing scanning socket thread";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ProcessControllerServerConnectionThread connectionThread(fdHolder, shutdownPipe);
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

TEST_F(TestProcessControllerServerConnectionThread, eof_while_running) //NOLINT
{
    const std::string expected = "ProcessController Connection closed: EOF";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ProcessControllerServerConnectionThread connectionThread(fdHolder, shutdownPipe);
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, send_zero_length) //NOLINT
{
    const std::string expected = "Ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ProcessControllerServerConnectionThread connectionThread(fdHolder, shutdownPipe);
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, closed_fd) //NOLINT
{
    const std::string expected = "Closing socket, error: 9";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    ProcessControllerServerConnectionThread connectionThread(fdHolder, shutdownPipe);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, over_max_length) //NOLINT
{
    const std::string expected = "Aborting ProcessController Connection Thread: failed to read length";

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ProcessControllerServerConnectionThread connectionThread(serverFd, shutdownPipe);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLength(clientFd.get(), 0x1000080);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, max_length) //NOLINT
{
    const std::string expected = "Aborting socket connection: failed to read entire message";

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ProcessControllerServerConnectionThread connectionThread(serverFd, shutdownPipe);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // length is limited to ~16MB
    unixsocket::writeLength(clientFd.get(), 0x100007f);
    ::close(clientFd.get());
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, corrupt_request) //NOLINT
{
    const std::string expected = "Terminated ProcessControllerServerConnectionThread with serialisation exception: ";

    const std::string request = { 0x01, 0x00 };

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ProcessControllerServerConnectionThread connectionThread(serverFd, shutdownPipe);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd.get(), request);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThreadWithSocketPair, send_shutdown_notification) // NOLINT
{
    const std::string expected = "Managed to get file descriptor: ";

    auto processControl = scan_messages::ProcessControlSerialiser();
    processControl.setCommandType(scan_messages::E_SHUTDOWN);

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    ProcessControllerServerConnectionThread connectionThread(m_serverFd, shutdownPipe);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd.get(), processControl.serialise());

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(shutdownPipe->notified());
}

TEST_F(TestProcessControllerServerConnectionThreadWithSocketPair, send_invalid_notification) // NOLINT
{
    const std::string expected = "Received unknown signal on Process Controller: 2";

    auto processControl = MockProcessControlSerialiser();
    processControl.setCommandTypeInt(2);

    auto shutdownPipe = std::make_shared<Common::Threads::NotifyPipe>();
    ProcessControllerServerConnectionThread connectionThread(m_serverFd, shutdownPipe);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(m_clientFd.get(), processControl.serialise());

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_FALSE(shutdownPipe->notified());
}
