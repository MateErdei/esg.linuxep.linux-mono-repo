//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"


#include "datatypes/sophos_filesystem.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/Reloader.h"
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

        fs::path m_testDir;
    };

    class MockScanner : public threat_scanner::IThreatScanner
    {
    public:
        MOCK_METHOD(scan_messages::ScanResponse, scan, (datatypes::AutoFd&, const std::string&, int64_t,
            const std::string& userID));

        MOCK_METHOD(std::string, susiErrorToReadableError, (const std::string& filePath, const std::string& susiError));
    };
    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD(threat_scanner::IThreatScannerPtr, createScanner, (bool scanArchives, bool scanImages));

        MOCK_METHOD(bool, update, ());
        MOCK_METHOD(ReloadResult, reload, ());
        MOCK_METHOD(void, shutdown, ());
        MOCK_METHOD(bool, susiIsInitialized, ());
    };
}

TEST_F(TestThreatDetectorSocket, test_construction) //NOLINT
{
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_NO_THROW(unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory));
}

TEST_F(TestThreatDetectorSocket, test_running) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();
    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("Scanning Server starting listening on socket"));
    EXPECT_TRUE(appenderContains("Closing Scanning Server socket"));
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

TEST_F(TestThreatDetectorSocket, test_scan_threat) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "THREAT","");

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _))
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
        EXPECT_EQ(response.getDetections()[0].name, "THREAT");
    }

    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("Scanning Server starting listening on socket:"));
    EXPECT_TRUE(appenderContains("Scanning Server accepting connection:"));
    EXPECT_TRUE(appenderContains("Scanning Server thread got connection"));
    EXPECT_TRUE(appenderContains("Closing Scanning Server socket"));
}

TEST_F(TestThreatDetectorSocket, test_scan_clean) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/bin/bash", "","");

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _))
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
    }

    server.requestStop();
    server.join();
}

TEST_F(TestThreatDetectorSocket, test_scan_twice) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/bin/bash", "","");

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _))
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

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _))
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
    PRINT("Starting " << clientConnectionCount << " client connections");

    ASSERT_GT(clientConnectionCount, 1);
    for (int i=0; i < clientConnectionCount; ++i)
    {
        auto& socket = client_sockets.emplace_back(socketPath);
        socket.connect();
    }

    ASSERT_FALSE(client_sockets.empty());
    // Can't continue test if we don't have refused connections
    ASSERT_TRUE(waitForLog("Refusing connection: Maximum number of scanners reached", 500ms));

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
