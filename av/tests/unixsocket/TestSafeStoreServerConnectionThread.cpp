// Copyright 2022-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "SafeStoreSocketMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"
#include "scan_messages/SampleThreatDetected.h"
#include "tests/common/MemoryAppender.h"
#include "tests/safestore/MockIQuarantineManager.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/safeStoreSocket/SafeStoreServerConnectionThread.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/MockSysCalls.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <memory>

namespace fs = sophos_filesystem;

using namespace safestore;
using namespace unixsocket;
using namespace ::testing;

namespace
{
    class TestSafeStoreServerConnectionThread : public SafeStoreSocketMemoryAppenderUsingTests
    {
    protected:
        TestSafeStoreServerConnectionThread() : memoryAppenderHolder(*this)
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

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir);
            appConfig.setData("PLUGIN_INSTALL", m_testDir);

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
        std::shared_ptr<MockIQuarantineManager> mockQuarantineManager_ = std::make_shared<MockIQuarantineManager>();
    };
} // namespace

TEST_F(TestSafeStoreServerConnectionThread, successful_construction)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(
        unixsocket::SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_sysCalls));
}

TEST_F(TestSafeStoreServerConnectionThread, isRunning_false_after_construction)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    unixsocket::SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_sysCalls);
    EXPECT_FALSE(connectionThread.isRunning());
}

TEST_F(TestSafeStoreServerConnectionThread, fail_construction_with_bad_fd)
{
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    EXPECT_THROW(
        unixsocket::SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_sysCalls),
        std::runtime_error);
}

TEST_F(TestSafeStoreServerConnectionThread, stop_while_running)
{
    const std::string expected = "Closing SafeStoreServerConnectionThread";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_sysCalls);
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

TEST_F(TestSafeStoreServerConnectionThread, eof_while_running)
{
    const std::string expected = "SafeStoreServerConnectionThread closed: EOF";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestSafeStoreServerConnectionThread, send_zero_length)
{
    const std::string expected = "SafeStoreServerConnectionThread ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestSafeStoreServerConnectionThread, bad_notify_pipe_fd)
{
    const std::string expected = "Closing SafeStoreServerConnectionThread, error from notify pipe";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[1].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr)).WillOnce(DoAll(SetArrayArgument<0>(fds, fds + 2), Return(1)));

    SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestSafeStoreServerConnectionThread, bad_socket_fd)
{
    const std::string expected = "Closing SafeStoreServerConnectionThread, error from socket";
    UsingMemoryAppender memoryAppenderHolder(*this);

    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[0].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCalls, ppoll(_, 2, _, nullptr)).WillOnce(DoAll(SetArrayArgument<0>(fds, fds + 2), Return(1)));

    SafeStoreServerConnectionThread connectionThread(fdHolder, mockQuarantineManager_, m_mockSysCalls);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestSafeStoreServerConnectionThread, over_max_length)
{
    const std::string expected = "Aborting SafeStoreServerConnectionThread: message too big";

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    SafeStoreServerConnectionThread connectionThread(serverFd, mockQuarantineManager_, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLength(clientFd.get(), 0x1000080);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestSafeStoreServerConnectionThread, corrupt_request)
{
    const std::string expected = "Aborting SafeStoreServerConnectionThread: failed to parse detection";

    const std::string request = { 0x01, 0x00 };

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    SafeStoreServerConnectionThread connectionThread(serverFd, mockQuarantineManager_, m_sysCalls);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd.get(), request);
    EXPECT_TRUE(waitForLog(expected));
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestSafeStoreServerConnectionThread, ReceivesRequestAndCallsQuarantineManager)
{
    // Create socket pair
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);

    const std::string filePath = "/path/to/eicar.txt";
    const std::string threatId = "01234567-89ab-cdef-0123-456789abcdef";
    const std::string threatType = "virus";
    const std::string threatName = "EICAR-AV-Test";
    const std::string threatSha256 = "494aaaff46fa3eac73ae63ffbdfd8267131f95c51cc819465fa1797f6ccacf9d";
    const std::string sha256 = "131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267";
    const std::string correlationId = "fedcba98-7654-3210-fedc-ba9876543210";

    auto detection = createThreatDetectedWithRealFd({ .threatType = threatType,
                                                      .threatName = threatName,
                                                      .filePath = filePath,
                                                      .sha256 = sha256,
                                                      .threatSha256 = threatSha256,
                                                      .threatId = threatId,
                                                      .correlationId = correlationId });

    EXPECT_CALL(
        *mockQuarantineManager_,
        quarantineFile(filePath, threatId, threatType, threatName, threatSha256, sha256, correlationId, _));

    SafeStoreServerConnectionThread connectionThread(serverFd, mockQuarantineManager_, m_sysCalls);
    connectionThread.start();

    // Send detection
    writeLengthAndBuffer(clientFd.get(), detection.serialise());
    send_fd(clientFd.get(), detection.autoFd.get());

    EXPECT_TRUE(waitForLog("Received Threat:"));

    connectionThread.requestStop();
    connectionThread.join();
}
