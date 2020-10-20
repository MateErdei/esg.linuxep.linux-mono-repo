/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/SocketUtils.h"

#include "tests/common/MemoryAppender.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace unixsocket;

namespace
{
    class TestScanningServerConnectionThread : public UnixSocketMemoryAppenderUsingTests
    {};

    class MockScanner : public threat_scanner::IThreatScanner
    {
    public:
        MOCK_METHOD4(scan, scan_messages::ScanResponse(datatypes::AutoFd&, const std::string&, int64_t,
                const std::string& userID));
    };
    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD1(createScanner, threat_scanner::IThreatScannerPtr(bool scanArchives));
    };

}

using namespace ::testing;

TEST_F(TestScanningServerConnectionThread, successful_construction) //NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(unixsocket::ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory));
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_bad_fd) //NOLINT
{
    using namespace unixsocket;
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, scannerFactory), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, fail_construction_with_null_factory) //NOLINT
{
    using namespace unixsocket;
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, nullptr), std::runtime_error);
}

TEST_F(TestScanningServerConnectionThread, stop_while_running) //NOLINT
{
    const std::string expected = "Closing scanning socket thread";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory);
    EXPECT_FALSE(connectionThread.isRunning());
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // No waiting...
    connectionThread.requestStop();
    connectionThread.join();
    EXPECT_FALSE(connectionThread.isRunning());

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, eof_while_running) //NOLINT
{
    const std::string expected = "Scanning Server Connection closed: EOF";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory);
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, send_zero_length) //NOLINT
{
    const std::string expected = "Ignoring length of zero / No new messages";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory, 1);
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, closed_fd) //NOLINT
{
    const std::string expected = "Closing socket because pselect failed: 9";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/zero", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    int fd = fdHolder.get();
    ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory);
    ::close(fd); // fd in connection Thread now broken
    connectionThread.start();
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, over_max_length) //NOLINT
{
    const std::string expected = "Aborting Scanning Server Connection Thread: failed to read length";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLength(clientFd, 0x1000080);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, max_length) //NOLINT
{
    const std::string expected = "Aborting socket connection: failed to read entire message";
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    // length is limited to ~16MB
    unixsocket::writeLength(clientFd, 0x100007f);
    ::close(clientFd);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, corrupt_request) //NOLINT
{
    const std::string expected = "Terminated ScanningServerConnectionThread with serialisation exception: ";
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string request = { 0x01, 0x00 };

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, valid_request_no_fd) //NOLINT
{
    const std::string expected = "Aborting socket connection: failed to read fd";
    UsingMemoryAppender memoryAppenderHolder(*this);

    scan_messages::ClientScanRequest request;
    request.setPath("/file/to/scan");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request.serialise());
    ::close(clientFd);
    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, send_fd) //NOLINT
{
    const std::string expected = "Managed to get file descriptor: ";
    UsingMemoryAppender memoryAppenderHolder(*this);

    scan_messages::ClientScanRequest request;
    request.setPath("/file/to/scan");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();
    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "THREAT");

    testing::Mock::AllowLeak(scanner.get());

    EXPECT_CALL(*scanner, scan(_, "/file/to/scan", _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request.serialise());
    datatypes::AutoFd tmpFile(::open(".", O_TMPFILE | O_RDWR, 00700));
    ASSERT_GE(tmpFile.get(), 0);
    ret = send_fd(clientFd, tmpFile.get()); // send a valid file descriptor
    tmpFile.close();
    ASSERT_GE(ret, 0);
    int length = unixsocket::readLength(clientFd);
    static_cast<void>(length);

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, fd_not_readable) //NOLINT
{
    const std::string expected = "Aborting socket connection: fd is not open for read";
    UsingMemoryAppender memoryAppenderHolder(*this);

    scan_messages::ClientScanRequest request;
    request.setPath("/file/to/scan");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request.serialise());
    datatypes::AutoFd tmpFile(::open(".", O_TMPFILE | O_WRONLY, 00700));
    ASSERT_GE(tmpFile.get(), 0);
    ret = send_fd(clientFd, tmpFile.get());
    tmpFile.close();
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, fd_is_device) //NOLINT
{
    const std::string expected = "Aborting socket connection: fd is not a regular file";
    UsingMemoryAppender memoryAppenderHolder(*this);

    scan_messages::ClientScanRequest request;
    request.setPath("/file/to/scan");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request.serialise());
    datatypes::AutoFd devNull(::open("/dev/null", O_RDONLY));
    ASSERT_GE(devNull.get(), 0);
    ret = send_fd(clientFd, devNull.get());
    devNull.close();
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, fd_is_socket) //NOLINT
{
    const std::string expected = "Aborting socket connection: fd is not a regular file";
    UsingMemoryAppender memoryAppenderHolder(*this);

    scan_messages::ClientScanRequest request;
    request.setPath("/file/to/scan");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request.serialise());
    ret = send_fd(clientFd, clientFd.get());
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestScanningServerConnectionThread, fd_is_path) //NOLINT
{
    const std::string expected = "Aborting socket connection: fd is not open for read";
    UsingMemoryAppender memoryAppenderHolder(*this);

    scan_messages::ClientScanRequest request;
    request.setPath("/file/to/scan");
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");

    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    int socket_fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fds);
    ASSERT_EQ(ret, 0);
    datatypes::AutoFd serverFd(socket_fds[0]);
    datatypes::AutoFd clientFd(socket_fds[1]);
    ASSERT_GE(serverFd.get(), 0);
    ASSERT_GE(clientFd.get(), 0);
    ScanningServerConnectionThread connectionThread(serverFd, scannerFactory);
    connectionThread.start();
    EXPECT_TRUE(connectionThread.isRunning());
    unixsocket::writeLengthAndBuffer(clientFd, request.serialise());
    datatypes::AutoFd devNull(::open("/etc/passwd", O_PATH));
    ASSERT_GE(devNull.get(), 0);
    ret = send_fd(clientFd, devNull.get());
    devNull.close();
    ASSERT_GE(ret, 0);

    waitForLog(expected);
    connectionThread.requestStop();
    connectionThread.join();

    EXPECT_GT(m_memoryAppender->size(), 0);
    EXPECT_TRUE(appenderContains(expected));
}
