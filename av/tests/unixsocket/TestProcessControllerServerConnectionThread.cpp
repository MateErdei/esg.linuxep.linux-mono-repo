// Copyright 2021-2022, Sophos Limited.  All rights reserved.

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
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        UsingMemoryAppender memoryAppenderHolder;
        fs::path m_testDir;
    };


    class MockCallback : public IProcessControlMessageCallback
    {
    public:
        void processControlMessage(const scan_messages::E_COMMAND_TYPE& /*command*/) override
        {

        }
    };
}

TEST_F(TestProcessControllerServerConnectionThread, successful_construction) //NOLINT
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    EXPECT_NO_THROW(unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, callback));
}

TEST_F(TestProcessControllerServerConnectionThread, isRunning_false_after_construction) //NOLINT
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, callback);
    EXPECT_FALSE(connectionThread.isRunning());
}

TEST_F(TestProcessControllerServerConnectionThread, fail_construction_with_bad_fd) //NOLINT
{
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    EXPECT_THROW(unixsocket::ProcessControllerServerConnectionThread connectionThread(fdHolder, callback),
                 std::runtime_error);
}

TEST_F(TestProcessControllerServerConnectionThread, stop_while_running) //NOLINT
{
    const std::string expected = "Closing Process Controller connection thread";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback);
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
    const std::string expected = "Process Controller connection thread closed: EOF";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, send_zero_length) //NOLINT
{
    const std::string expected = "Ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, closed_fd) //NOLINT
{
    const std::string expected = "Closing Process Controller connection thread, error: 9";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(fdHolder, callback);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, over_max_length) //NOLINT
{
    const std::string expected = "Aborting Process Controller connection thread: failed to read length";

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLength(clientFd.get(), 0x1000080);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, max_length) //NOLINT
{
    const std::string expected = "Process Controller connection thread aborting socket connection: failed to read entire message";

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    std::shared_ptr<MockCallback> callback = std::make_shared<MockCallback>();
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // length is limited to ~16MB
    unixsocket::writeLength(clientFd.get(), 0x100007f);
    ::close(clientFd.get());
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestProcessControllerServerConnectionThread, corrupt_request) //NOLINT
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
    ProcessControllerServerConnectionThread connectionThread(serverFd, callback);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd.get(), request);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}