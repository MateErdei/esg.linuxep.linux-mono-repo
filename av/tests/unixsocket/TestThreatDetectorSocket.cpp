//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "capnp/message.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/Reloader.h"
#include "tests/common/MockScanner.h"
#include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/SocketUtils.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <list>
#include <memory>

using namespace ::testing;
namespace fs = sophos_filesystem;

namespace
{
    class TestThreatDetectorSocket : public UnixSocketMemoryAppenderUsingTests
    {
    protected:
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
        static scan_messages::ScanRequest makeScanRequestObject(const std::string& path)
        {
            ::capnp::MallocMessageBuilder message;
            Sophos::ssplav::FileScanRequest::Builder requestBuilder =
                message.initRoot<Sophos::ssplav::FileScanRequest>();
            requestBuilder.setPathname(path);
            requestBuilder.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
            requestBuilder.setUserID("n/a");

            Sophos::ssplav::FileScanRequest::Reader requestReader = requestBuilder;

            return scan_messages::ScanRequest(requestReader);
        }
        fs::path m_testDir;
    };
}

TEST_F(TestThreatDetectorSocket, test_construction)
{
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_NO_THROW(unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory));
}

TEST_F(TestThreatDetectorSocket, test_running)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();
    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("ScanningServer starting listening on socket"));
    EXPECT_TRUE(appenderContains("Closing ScanningServer"));
}

static scan_messages::ScanResponse scan(
    unixsocket::ScanningClientSocket& socket,
    int file_fd,
    const std::string& filename)
{
    scan_messages::ScanResponse response;

    if (socket.socketFd() <= 0)
    {
        auto connectResult = socket.connect();
        if (connectResult != 0)
        {
            response.setErrorMsg("Failed to connect");
            return response;
        }
    }

    auto request = std::make_shared<scan_messages::ClientScanRequest>();
    request->setPath(filename);
    request->setScanInsideArchives(false);
    request->setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request->setUserID("root");
    request->setFd(file_fd);

    auto sendResult = socket.sendRequest(request);
    if (!sendResult)
    {
        response.setErrorMsg("Failed to send scan request");
        return response;
    }

    auto receiveResponse = socket.receiveResponse(response);
    if (!receiveResponse)
    {
        response.setErrorMsg("Failed to get scan response");
        return response;
    }

    return response;
}

TEST_F(TestThreatDetectorSocket, test_scan_threat)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "THREAT","");
    scan_messages::ScanRequest request = makeScanRequestObject(THREAT_PATH);
    EXPECT_CALL(*scanner, scan(_, Eq(std::ref(request))))
        .WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false))
        .WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        auto fd =testFile.open();
        auto response = scan(client_socket, fd, THREAT_PATH);

        EXPECT_FALSE(response.allClean());
        EXPECT_EQ(response.getErrorMsg(), "");
        ASSERT_EQ(response.getDetections().size(), 1);
        EXPECT_EQ(response.getDetections()[0].name, "THREAT");
    }

    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("ScanningServer starting listening on socket:"));
    EXPECT_TRUE(appenderContains("ScanningServer accepting connection:"));
    EXPECT_TRUE(appenderContains("ScanningServerConnectionThread got connection"));
    EXPECT_TRUE(appenderContains("Closing ScanningServer socket"));
}

TEST_F(TestThreatDetectorSocket, test_scan_clean)
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();

    scan_messages::ScanRequest request = makeScanRequestObject(THREAT_PATH);
    EXPECT_CALL(*scanner, scan(_, Eq(std::ref(request))))
        .WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false))
        .WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        auto fd = testFile.open();
        auto response = scan(client_socket, fd, THREAT_PATH);

        EXPECT_TRUE(response.allClean());
        EXPECT_EQ(response.getDetections().size(), 0);
        EXPECT_EQ(response.getErrorMsg(), "");
    }

    server.requestStop();
    server.join();
}

TEST_F(TestThreatDetectorSocket, test_scan_twice)
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/bin/bash", "","");


    scan_messages::ScanRequest request = makeScanRequestObject(THREAT_PATH);
    EXPECT_CALL(*scanner, scan(_, Eq(std::ref(request))))
        .WillRepeatedly(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false))
        .WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        auto fd = testFile.open();
        ASSERT_GE(fd, 0);
        auto response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_TRUE(response.allClean());

        fd = testFile.open();
        ASSERT_GE(fd, 0);
        response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_TRUE(response.allClean());
    }

    server.requestStop();
    server.join();
}

TEST_F(TestThreatDetectorSocket, test_scan_throws)
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto* scannerPtr = scanner.get();


    scan_messages::ScanRequest request = makeScanRequestObject(THREAT_PATH);
    EXPECT_CALL(*scanner, scan(_, Eq(std::ref(request))))
        .WillRepeatedly(Throw(std::runtime_error("Intentional throw")));
    EXPECT_CALL(*scannerFactory, createScanner(false, false))
        .WillOnce(Return(ByMove(std::move(scanner))))
        .WillRepeatedly([](bool, bool)->threat_scanner::IThreatScannerPtr{ return nullptr; });

    unixsocket::ScanningServerSocket server(socketPath, 0600, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        auto fd = testFile.open();
        auto response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_NE(response.getErrorMsg(), "");
    }

    server.requestStop();
    server.join();

    testing::Mock::VerifyAndClearExpectations(scannerPtr);
    testing::Mock::VerifyAndClearExpectations(scannerFactory.get());
    scannerFactory.reset();
}

TEST_F(TestThreatDetectorSocket, test_too_many_connections_are_refused)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "/tmp/scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, createScanner(false, false))
        .WillRepeatedly(
            [](bool, bool) -> threat_scanner::IThreatScannerPtr
            { return std::make_unique<StrictMock<MockScanner>>(); });

    unixsocket::ScanningServerSocket server(socketPath, 0600, scannerFactory);
    server.start();

    // Use a std::list since we can't copy/move ScanningClientSocket
    std::list<unixsocket::ScanningClientSocket> client_sockets;
    // Create client connections - more than the max
    int clientConnectionCount = server.maxClientConnections() + 1;
    LOG("Starting " << clientConnectionCount << " client connections");

    ASSERT_GT(clientConnectionCount, 1);
    for (int i=0; i < clientConnectionCount; ++i)
    {
        auto& socket = client_sockets.emplace_back(socketPath);
        socket.connect();
    }

    ASSERT_FALSE(client_sockets.empty());
    // Can't continue test if we don't have refused connections
    ASSERT_TRUE(waitForLog("ScanningServer refusing connection: Maximum number of scanners reached", 500ms));

    // Try a scan with the last connection
    {
        assert(!client_sockets.empty()); // make cppcheck happy
        unixsocket::ScanningClientSocket& client_socket(client_sockets.back());
        TestFile testFile("testfile");
        auto fd = testFile.open();

        auto response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_EQ(response.getErrorMsg(), "Failed to send scan request");
    }

    server.requestStop();
    client_sockets.clear();
    server.join();
}