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
#include "unixsocket/SocketUtils.h"
#include "unixsocket/metadataRescanSocket/MetadataRescanClientSocket.h"
#include "unixsocket/metadataRescanSocket/MetadataRescanServerConnectionThread.h"
#include "unixsocket/metadataRescanSocket/MetadataRescanServerSocket.h"

#include "Common/Helpers/MockSysCalls.h"

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
        scan_messages::MetadataRescan request_{ .filePath = "filePath",
                                                .sha256 = "sha256",
                                                .threat = {
                                                    .type = "threatType",
                                                    .name = "threatName",
                                                    .sha256 = "threatSha256",
                                                } };
        fs::path m_testDir;
        datatypes::AutoFd m_serverFd;
        datatypes::AutoFd m_clientFd;
        std::shared_ptr<Common::SystemCallWrapper::SystemCallWrapper> m_sysCallWrapper{
            std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>()
        };
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper{ std::make_shared<MockSystemCallWrapper>() };
        std::shared_ptr<MockScannerFactory> mockScannerFactory_ = std::make_shared<MockScannerFactory>();
        std::unique_ptr<MockScanner> mockScanner_ = std::make_unique<MockScanner>();
    };
} // namespace

TEST_F(TestMetadataRescanServerConnectionThread, SuccessfulConstruction)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(
        MetadataRescanServerConnectionThread connectionThread(fdHolder, mockScannerFactory_, m_sysCallWrapper));
}

TEST_F(TestMetadataRescanServerConnectionThread, FailConstructionWithBadFd)
{
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(fdHolder.get(), -1);
    EXPECT_THROW(
        MetadataRescanServerConnectionThread(fdHolder, mockScannerFactory_, m_sysCallWrapper), std::runtime_error);
}

TEST_F(TestMetadataRescanServerConnectionThread, FailConstructionWithNullFactory)
{
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    EXPECT_GE(fdHolder.get(), 0);
    EXPECT_THROW(MetadataRescanServerConnectionThread(fdHolder, nullptr, m_sysCallWrapper), std::runtime_error);
}

TEST_F(TestMetadataRescanServerConnectionThread, StopWhileRunning)
{
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    MetadataRescanServerConnectionThread connectionThread(fdHolder, mockScannerFactory_, m_sysCallWrapper);
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
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    MetadataRescanServerConnectionThread connectionThread(fdHolder, mockScannerFactory_, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(waitForLog("MetadataRescanServerConnectionThread closed: EOF"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, SendZeroLength)
{
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    MetadataRescanServerConnectionThread connectionThread(fdHolder, mockScannerFactory_, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(waitForLog("MetadataRescanServerConnectionThread ignoring length of zero / No new messages"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, BadNotifyPipeFd)
{
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[1].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds + 2), Return(1)));

    MetadataRescanServerConnectionThread connectionThread(fdHolder, mockScannerFactory_, m_mockSysCallWrapper);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog("Closing MetadataRescanServerConnectionThread, error from notify pipe"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, BadSocketFd)
{
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    struct pollfd fds[2]{};
    fds[0].revents = POLLERR;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds + 2), Return(1)));

    MetadataRescanServerConnectionThread connectionThread(fdHolder, mockScannerFactory_, m_mockSysCallWrapper);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    EXPECT_TRUE(waitForLog("Closing MetadataRescanServerConnectionThread, error from socket"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, CorruptRequest)
{
    const std::string request = { 0x01, 0x00 };

    MetadataRescanServerConnectionThread connectionThread(m_serverFd, mockScannerFactory_, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());

    ::send(m_clientFd, request.data(), request.size(), MSG_NOSIGNAL);
    EXPECT_TRUE(waitForLog("Terminated MetadataRescanServerConnectionThread with serialisation exception:"));
    connectionThread.requestStop();
    connectionThread.join();
}

TEST_F(TestMetadataRescanServerConnectionThread, SendRequest)
{
    EXPECT_CALL(*mockScanner_, metadataRescan).WillOnce(Return(MetadataRescanResponse::undetected));
    EXPECT_CALL(*mockScannerFactory_, createScanner).WillOnce(Return(ByMove(std::move(mockScanner_))));

    auto serialised = request_.Serialise();

    MetadataRescanServerConnectionThread connectionThread(m_serverFd, mockScannerFactory_, m_sysCallWrapper);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    writeLengthAndBuffer(*m_sysCallWrapper, m_clientFd, serialised);

    MetadataRescanResponse response;
    std::ignore = ::read(m_clientFd, &response, sizeof(MetadataRescanResponse));
    EXPECT_EQ(response, MetadataRescanResponse::undetected);

    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_TRUE(appenderContains(
        "DEBUG - MetadataRescanServerConnectionThread received a metadata rescan request of filePath=filePath, threatType=threatType, threatName=threatName, threatSHA256=threatSha256, SHA256=sha256"));
}

TEST_F(TestMetadataRescanServerConnectionThread, SendTwoRequests)
{
    EXPECT_CALL(*mockScanner_, metadataRescan)
        .WillOnce(Return(MetadataRescanResponse::threatPresent))
        .WillOnce(Return(MetadataRescanResponse::undetected));
    EXPECT_CALL(*mockScannerFactory_, createScanner).WillOnce(Return(ByMove(std::move(mockScanner_))));

    auto serialised = request_.Serialise();

    MetadataRescanServerConnectionThread connectionThread(m_serverFd, mockScannerFactory_, m_sysCallWrapper);
    connectionThread.start();

    {
        writeLengthAndBuffer(*m_sysCallWrapper, m_clientFd, serialised);
        MetadataRescanResponse response;
        std::ignore = ::read(m_clientFd, &response, sizeof(MetadataRescanResponse));
        EXPECT_EQ(response, MetadataRescanResponse::threatPresent);
    }

    {
        writeLengthAndBuffer(*m_sysCallWrapper, m_clientFd, serialised);
        MetadataRescanResponse response;
        std::ignore = ::read(m_clientFd, &response, sizeof(MetadataRescanResponse));
        EXPECT_EQ(response, MetadataRescanResponse::undetected);
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
        scan_messages::MetadataRescan request_{ .filePath = "filePath",
                                                .sha256 = "sha256",
                                                .threat = {
                                                    .type = "threatType",
                                                    .name = "threatName",
                                                    .sha256 = "threatSha256",
                                                } };
        std::shared_ptr<MockScannerFactory> mockScannerFactory_ = std::make_shared<MockScannerFactory>();
        std::unique_ptr<MockScanner> mockScanner_ = std::make_unique<MockScanner>();
    };
} // namespace

TEST_F(TestMetadataRescanServerSocket, Construction)
{
    EXPECT_NO_THROW(MetadataRescanServerSocket server(socketPath_, 0666, mockScannerFactory_));
}

TEST_F(TestMetadataRescanServerSocket, Running)
{
    MetadataRescanServerSocket server(socketPath_, 0666, mockScannerFactory_);
    server.start();
    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("MetadataRescanServer starting listening on socket"));
    EXPECT_TRUE(appenderContains("Closing MetadataRescanServer socket"));
}

TEST_F(TestMetadataRescanServerSocket, SuccessfulConnection)
{
    EXPECT_CALL(*mockScanner_, metadataRescan).WillOnce(Return(MetadataRescanResponse::threatPresent));
    EXPECT_CALL(*mockScannerFactory_, createScanner).WillOnce(Return(ByMove(std::move(mockScanner_))));

    MetadataRescanServerSocket server(socketPath_, 0666, mockScannerFactory_);
    server.start();

    {
        MetadataRescanClientSocket client_socket(socketPath_);
        client_socket.connect();
        client_socket.sendRequest(request_);
        MetadataRescanResponse response;
        auto result = client_socket.receiveResponse(response);
        EXPECT_TRUE(result);
        EXPECT_EQ(response, scan_messages::MetadataRescanResponse::threatPresent);
    }

    server.requestStop();
    server.join();
}

TEST_F(TestMetadataRescanServerSocket, ClientReceivesInvalidEnum)
{
    EXPECT_CALL(*mockScanner_, metadataRescan)
        .WillOnce(Return(static_cast<MetadataRescanResponse>(100)))
        .WillOnce(Return(static_cast<MetadataRescanResponse>(5)));
    EXPECT_CALL(*mockScannerFactory_, createScanner).WillOnce(Return(ByMove(std::move(mockScanner_))));

    MetadataRescanServerSocket server(socketPath_, 0666, mockScannerFactory_);
    server.start();

    {
        MetadataRescanClientSocket client_socket(socketPath_);
        client_socket.connect();

        client_socket.sendRequest(request_);
        MetadataRescanResponse response;
        auto result = client_socket.receiveResponse(response);
        EXPECT_FALSE(result);

        client_socket.sendRequest(request_);
        result = client_socket.receiveResponse(response);
        EXPECT_FALSE(result);

        EXPECT_TRUE(appenderContains("Invalid response"));
    }

    server.requestStop();
    server.join();
}
