// Copyright 2020-2023 Sophos Limited. All rights reserved.

#define AUTO_FD_IMPLICIT_INT

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "datatypes/sophos_filesystem.h"
#include "scan_messages/ScanRequest.h"
#include "tests/common/MemoryAppender.h"
#include "tests/common/MockScanner.h"
#include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"
#include "tests/datatypes/MockSysCalls.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/metadataRescanSocket/MetadataRescanClientSocket.h"
#include "unixsocket/metadataRescanSocket/MetadataRescanServerConnectionThread.h"
#include "unixsocket/metadataRescanSocket/MetadataRescanServerSocket.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <capnp/message.h>
#include <gtest/gtest.h>

#include <list>
#include <memory>

using namespace unixsocket;
using namespace ::testing;

namespace fs = sophos_filesystem;

using scan_messages::MetadataRescan, scan_messages::MetadataRescanResponse;

namespace
{

    class TestMetadataRescanServerConnectionThread : public UnixSocketMemoryAppenderUsingTests
    {
    protected:
        TestMetadataRescanServerConnectionThread() : memoryAppenderHolder_(*this)
        {
            request.setPath("/file/to/scan");
            request.setScanInsideArchives(false);
            request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
            request.setUserID("root");
        }

        void SetUp() override
        {
            m_testDir = createTestSpecificDirectory();
            fs::current_path(m_testDir);

            int socket_fds[2];
            int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
            ASSERT_EQ(ret, 0);
            m_serverFd.reset(socket_fds[0]);
            m_clientFd.reset(socket_fds[1]);
            ASSERT_GE(m_serverFd.get(), 0);
            ASSERT_GE(m_clientFd.get(), 0);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        UsingMemoryAppender memoryAppenderHolder_;
        scan_messages::ClientScanRequest request;
        scan_messages::MetadataRescan request_{ .threatType = "threatType",
                                                .threatName = "threatName",
                                                .filePath = "filePath",
                                                .sha256 = "sha256" };
        fs::path m_testDir;
        datatypes::AutoFd m_serverFd;
        datatypes::AutoFd m_clientFd;
        std::shared_ptr<datatypes::SystemCallWrapper> m_sysCallWrapper{
            std::make_shared<datatypes::SystemCallWrapper>()
        };
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper{ std::make_shared<MockSystemCallWrapper>() };
    };
} // namespace

TEST_F(TestMetadataRescanServerConnectionThread, SuccessfulConstruction)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(MetadataRescanServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCallWrapper));
}

TEST_F(TestMetadataRescanServerConnectionThread, FailConstructionWithBadFd)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(fdHolder.get(), -1);
    EXPECT_THROW(MetadataRescanServerConnectionThread(fdHolder, scannerFactory, m_sysCallWrapper), std::runtime_error);
}

TEST_F(TestMetadataRescanServerConnectionThread, FailConstructionWithNullFactory)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_GE(fdHolder.get(), 0);
    EXPECT_THROW(MetadataRescanServerConnectionThread(fdHolder, nullptr, m_sysCallWrapper), std::runtime_error);
}

TEST_F(TestMetadataRescanServerConnectionThread, StopWhileRunning)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    MetadataRescanServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCallWrapper);
    EXPECT_FALSE(connectionThread.isRunning());
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    connectionThread.requestStop();
    connectionThread.join();
    EXPECT_FALSE(connectionThread.isRunning());
    EXPECT_TRUE(appenderContains("Stopping MetadataRescanServerConnectionThread"));
}

TEST_F(TestMetadataRescanServerConnectionThread, EofWhileRunning)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    MetadataRescanServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(waitForLog("MetadataRescanServerConnectionThread closed: EOF"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, SendZeroLength)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    MetadataRescanServerConnectionThread connectionThread(fdHolder, scannerFactory, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(waitForLog("MetadataRescanServerConnectionThread ignoring length of zero / No new messages"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, BadNotifyPipeFd)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[1].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds + 2), Return(1)));

    MetadataRescanServerConnectionThread connectionThread(fdHolder, scannerFactory, m_mockSysCallWrapper);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog("Closing MetadataRescanServerConnectionThread, error from notify pipe"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, BadSocketFd)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[0].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds + 2), Return(1)));

    MetadataRescanServerConnectionThread connectionThread(fdHolder, scannerFactory, m_mockSysCallWrapper);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog("Closing MetadataRescanServerConnectionThread, error from socket"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, CorruptRequest)
{
    const std::string request = { 0x01, 0x00 };

    auto scannerFactory = std::make_shared<MockScannerFactory>();
    MetadataRescanServerConnectionThread connectionThread(m_serverFd, scannerFactory, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());

    ::send(m_clientFd, request.data(), request.size(), MSG_NOSIGNAL);
    EXPECT_TRUE(waitForLog("Terminated MetadataRescanServerConnectionThread with serialisation exception:"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, SendRequest)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();

    auto serialised = request_.Serialise();

    MetadataRescanServerConnectionThread connectionThread(m_serverFd, scannerFactory, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    writeLengthAndBuffer(m_clientFd, serialised);

    MetadataRescanResponse response;
    ::read(m_clientFd, &response, sizeof(MetadataRescanResponse));
    EXPECT_EQ(response, MetadataRescanResponse::ok);

    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_TRUE(appenderContains(
        "DEBUG - MetadataRescanServerConnectionThread received a metadata rescan request of filePath=filePath, threatType=threatType, threatName=threatName, SHA256=sha256"));
}

TEST_F(TestMetadataRescanServerConnectionThread, SendTwoRequests)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();

    auto serialised = request_.Serialise();

    MetadataRescanServerConnectionThread connectionThread(m_serverFd, scannerFactory, m_sysCallWrapper);
    connectionThread.start();

    {
        writeLengthAndBuffer(m_clientFd, serialised);
        MetadataRescanResponse response;
        ::read(m_clientFd, &response, sizeof(MetadataRescanResponse));
        EXPECT_EQ(response, MetadataRescanResponse::ok);
    }

    {
        writeLengthAndBuffer(m_clientFd, serialised);
        MetadataRescanResponse response;
        ::read(m_clientFd, &response, sizeof(MetadataRescanResponse));
        EXPECT_EQ(response, MetadataRescanResponse::ok);
    }

    connectionThread.requestStop();
    connectionThread.join();
}

namespace
{
    class TestMetadataRescanServerSocket : public UnixSocketMemoryAppenderUsingTests
    {
    protected:
        TestMetadataRescanServerSocket() : memoryAppenderHolder_{ *this }
        {
        }

        void SetUp() override
        {
            m_testDir = createTestSpecificDirectory();
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        UsingMemoryAppender memoryAppenderHolder_;
        fs::path m_testDir;
        std::string socketPath_{ "metadata_rescan_socket" };
        scan_messages::MetadataRescan request_{ .threatType = "threatType",
                                                .threatName = "threatName",
                                                .filePath = "filePath",
                                                .sha256 = "sha256" };
    };
} // namespace

TEST_F(TestMetadataRescanServerSocket, Construction)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();
    EXPECT_NO_THROW(MetadataRescanServerSocket server(socketPath_, 0666, scannerFactory));
}

TEST_F(TestMetadataRescanServerSocket, Running)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();
    MetadataRescanServerSocket server(socketPath_, 0666, scannerFactory);
    server.start();
    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("MetadataRescanServer starting listening on socket"));
    EXPECT_TRUE(appenderContains("Closing MetadataRescanServer socket"));
}

TEST_F(TestMetadataRescanServerSocket, SuccessfulConnection)
{
    auto scannerFactory = std::make_shared<MockScannerFactory>();

    MetadataRescanServerSocket server(socketPath_, 0666, scannerFactory);
    server.start();

    {
        MetadataRescanClientSocket client_socket(socketPath_);
        client_socket.connect();
        client_socket.sendRequest(request_);
        MetadataRescanResponse response;
        auto result = client_socket.receiveResponse(response);
        EXPECT_TRUE(result);
        EXPECT_EQ(response, scan_messages::MetadataRescanResponse::ok);
    }

    server.requestStop();
    server.join();
}
