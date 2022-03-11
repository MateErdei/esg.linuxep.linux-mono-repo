/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "common/AbortScanException.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/Reloader.h"
#include "unixsocket/threatDetectorSocket/ReconnectSettings.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include "unixsocket/SocketUtils.h"

#include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <list>
#include <memory>

#include <fcntl.h>

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
        MOCK_METHOD4(scan, scan_messages::ScanResponse(datatypes::AutoFd&, const std::string&, int64_t,
            const std::string& userID));

        MOCK_METHOD2(susiErrorToReadableError, std::string(const std::string& filePath, const std::string& susiError));
    };
    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD2(createScanner, threat_scanner::IThreatScannerPtr(bool scanArchives, bool scanImages));

        MOCK_METHOD0(update, bool());
        MOCK_METHOD0(reload, bool());
        MOCK_METHOD0(susiIsInitialized, bool());
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

static scan_messages::ScanResponse scan(unixsocket::ScanningClientSocket& socket, datatypes::AutoFd& file_fd, const std::string& filename)
{
    scan_messages::ClientScanRequest request;
    request.setPath(filename);
    request.setScanInsideArchives(false);
    request.setScanType(scan_messages::E_SCAN_TYPE_ON_DEMAND);
    request.setUserID("root");
    return socket.scan(file_fd, request);
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

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
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

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
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

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillRepeatedly(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false, false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
        ASSERT_GE(fd.get(), 0);
        auto response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_TRUE(response.allClean());

        fd.reset(testFile.open());
        ASSERT_GE(fd.get(), 0);
        response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_TRUE(response.allClean());
    }

    server.requestStop();
    server.join();
}


TEST_F(TestThreatDetectorSocket, test_scan_throws) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto* scannerPtr = scanner.get();

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillRepeatedly(Throw(std::runtime_error("Intentional throw")));
    EXPECT_CALL(*scannerFactory, createScanner(false, false))
            .WillOnce(Return(ByMove(std::move(scanner))))
            .WillRepeatedly([](bool, bool)->threat_scanner::IThreatScannerPtr{ return nullptr; });

    unixsocket::ScanningServerSocket server(socketPath, 0600, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath, {0, 1'000'000});
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
        auto response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_NE(response.getErrorMsg(), "");
    }

    server.requestStop();
    server.join();

    testing::Mock::VerifyAndClearExpectations(scannerPtr);
    testing::Mock::VerifyAndClearExpectations(scannerFactory.get());
    scannerFactory.reset();
}



TEST_F(TestThreatDetectorSocket, test_too_many_connections_are_refused) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "/tmp/scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, createScanner(false, false))
        .WillRepeatedly([](bool, bool)->threat_scanner::IThreatScannerPtr { return std::make_unique<StrictMock<MockScanner>>(); } );

    unixsocket::ScanningServerSocket server(socketPath, 0600, scannerFactory);
    server.start();

    struct timespec clientSleepTime={0, 10'000};

    // Use a std::list since we can't copy/move ScanningClientSocket
    std::list<unixsocket::ScanningClientSocket> client_sockets;
    // Create client connections - more than the max
    int clientConnectionCount = server.maxClientConnections() * 2;
    ASSERT_GT(clientConnectionCount, 0);
    for (int i=0; i < clientConnectionCount; ++i)
    {
        client_sockets.emplace_back(socketPath,clientSleepTime);
    }

    // Can't continue test if we don't have refused connections
    ASSERT_TRUE(appenderContains("Refusing connection: Maximum number of scanners reached"));
    ASSERT_FALSE(client_sockets.empty());

    // Try a scan with the last connection
    {
        assert(!client_sockets.empty()); // make cppcheck happy
        unixsocket::ScanningClientSocket& client_socket(client_sockets.back());
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
        for (int i=0;i< TOTAL_MAX_RECONNECTS / MAX_SCAN_RETRIES;++i)
        {
            auto response = scan(client_socket, fd, THREAT_PATH);
            EXPECT_NE(response.getErrorMsg(), ""); // We should have an error message
        }

        // 5th time should throw exception ( TOTAL_MAX_RECONNECTS / MAX_SCAN_RETRIES + 1)
        try
        {
            auto response = scan(client_socket, fd, THREAT_PATH);
            FAIL() << "Managed to scan 5 times with no connection";
        }
        catch (const AbortScanException&)
        {
            // expected exception
        }
    }

    server.requestStop();
    client_sockets.clear();
    server.join();
}


