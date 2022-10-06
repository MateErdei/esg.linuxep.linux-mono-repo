// Copyright 2022, Sophos Limited.  All rights reserved.

#include "UnixSocketMemoryAppenderUsingTests.h"

// #include "datatypes/sophos_filesystem.h"
// #include "scan_messages/ThreatDetected.h"
// #include "sophos_threat_detector/sophosthreatdetectorimpl/Reloader.h"
#include "scan_messages/ThreatDetected.h"
#include "tests/common/Common.h"
// #include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"
#include "unixsocket/IMessageCallback.h"
#include "unixsocket/safeStoreSocket/SafeStoreClient.h"
#include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"
// #include "unixsocket/SocketUtils.h"
// #include "unixsocket/safeStoreSocket/SafeStoreClient.h"
// #include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"
// #include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
// #include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
// #include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

// #include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

// #include <gmock/gmock.h>
// #include <gtest/gtest.h>
// #include <unixsocket/threatReporterSocket/ThreatReporterClient.h>
//
// #include <list>
// #include <memory>

#include <sys/fcntl.h>

using namespace testing;
using namespace scan_messages;
namespace fs = sophos_filesystem;

namespace
{
    class TestSafeStoreSocket : public UnixSocketMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setupFakeSafeStoreConfig();
            m_socketPath = pluginInstall() / "var/safestore_socket";
            m_userID = std::getenv("USER");
            m_threatName = "EICAR";
            m_threatPath = "/path/to/unit-test-eicar";
            m_sha256 = "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def";
            m_threatId = "Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350";
        }

        void TearDown() override
        {
            fs::remove_all(tmpdir());
        }

        ThreatDetected createThreatDetected()
        {
            std::time_t detectionTimeStamp = std::time(nullptr);

            return scan_messages::ThreatDetected(
                m_userID,
                detectionTimeStamp,
                E_VIRUS_THREAT_TYPE,
                m_threatName,
                E_SCAN_TYPE_ON_ACCESS_OPEN,
                E_NOTIFICATION_STATUS_CLEANED_UP,
                m_threatPath,
                E_SMT_THREAT_ACTION_SHRED,
                m_sha256,
                m_threatId,
                datatypes::AutoFd(open("/dev/zero", O_RDONLY)));
        }

        std::string m_threatPath;
        std::string m_threatName;
        std::string m_userID;
        std::string m_socketPath;
        std::string m_sha256;
        std::string m_threatId;
    };

    class MockQuarantineManager : public safestore::IQuarantineManager
    {
    public:
        MOCK_METHOD(
            bool,
            quarantineFile,
            (const std::string& filePath,
             const std::string& threatId,
             const std::string& threatName,
             const std::string& sha256,
             datatypes::AutoFd autoFd));
    };
} // namespace

TEST_F(TestSafeStoreSocket, a) // NOLINT
{
    auto quarantineManager = std::make_shared<MockQuarantineManager>();
    EXPECT_NO_THROW(unixsocket::SafeStoreServerSocket server(m_socketPath, 0666, quarantineManager));
}

TEST_F(TestSafeStoreSocket, blah) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto quarantineManager = std::make_shared<MockQuarantineManager>();
    unixsocket::SafeStoreServerSocket server(m_socketPath, 0666, quarantineManager);
    server.start();
    server.requestStop();
    server.join();

    EXPECT_TRUE(appenderContains("SafeStore Server starting listening on socket"));
    EXPECT_TRUE(appenderContains("Closing SafeStore Server socket"));
}

TEST_F(TestSafeStoreSocket, TestSendThreatReport) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    setupFakeSophosThreatDetectorConfig();

    {
        WaitForEvent serverWaitGuard;

        auto quarantineManager = std::make_shared<MockQuarantineManager>();
        EXPECT_CALL(*quarantineManager, quarantineFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(InvokeWithoutArgs(
                [&serverWaitGuard]()
                {
                    serverWaitGuard.onEventNoArgs();
                    return false;
                }));

        unixsocket::SafeStoreServerSocket server(m_socketPath, 0666, quarantineManager);

        server.start();

        // connect after we start
        unixsocket::SafeStoreClient client(m_socketPath);

        auto threatDetected = createThreatDetected();
        client.sendQuarantineRequest(threatDetected);

        serverWaitGuard.wait();

        // destructor will stop the thread
    }

    EXPECT_TRUE(appenderContains("SafeStore Server starting listening on socket:"));
    EXPECT_TRUE(appenderContains("SafeStore Server accepting connection:"));
    EXPECT_TRUE(appenderContains("SafeStore Server thread got connection"));
    EXPECT_TRUE(appenderContains("Read capn of"));
    EXPECT_TRUE(appenderContains("Managed to get file descriptor:"));
    EXPECT_TRUE(appenderContains("Received Threat:"));
    EXPECT_TRUE(appenderContains("File path: " + m_threatPath));
    EXPECT_TRUE(appenderContains("Threat ID: " + m_threatId));
    EXPECT_TRUE(appenderContains("Threat name: " + m_threatName));
    EXPECT_TRUE(appenderContains("SHA256: " + m_sha256));
    EXPECT_TRUE(appenderContains("File descriptor:"));
    EXPECT_TRUE(appenderContains("Closing SafeStore Server socket"));
    EXPECT_TRUE(appenderContains("Closing SafeStore connection thread"));
}

// TEST_F(TestSafeStoreSocket, TestSendTwoThreatReports) // NOLINT
//{
//     setupFakeSophosThreatDetectorConfig();
//     WaitForEvent serverWaitGuard;
//     WaitForEvent serverWaitGuard2;
//
//     std::time_t detectionTimeStamp = std::time(nullptr);
//
//     auto mockThreatReportCallback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
//

TEST_F(TestSafeStoreSocket, testClientSocketTriesToReconnect) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    unixsocket::SafeStoreClient client(m_socketPath, { 0, 0 });

    EXPECT_TRUE(appenderContains("Failed to connect to SafeStore - retrying after sleep", 9));
    EXPECT_TRUE(appenderContains("Reached total maximum number of connection attempts."));
}

// TEST_F(TestThreatDetectorSocket, test_scan_threat) // NOLINT
//{
//     UsingMemoryAppender memoryAppenderHolder(*this);
//
//     static const std::string THREAT_PATH = "/dev/null";
//     std::string socketPath = "scanning_socket";
//     auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
//     auto scanner = std::make_unique<StrictMock<MockScanner>>();
//
//     auto expected_response = scan_messages::ScanResponse();
//     expected_response.addDetection("/tmp/eicar.com", "THREAT", "");
//
//     EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
//     EXPECT_CALL(*scannerFactory, createScanner(false, false)).WillOnce(Return(ByMove(std::move(scanner))));
//
//     unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
//     server.start();
//
//     // Create client connection
//     {
//         unixsocket::ScanningClientSocket client_socket(socketPath);
//         TestFile testFile("testfile");
//         auto fd = testFile.open();
//         auto response = scan(client_socket, fd, THREAT_PATH);
//
//         EXPECT_FALSE(response.allClean());
//         EXPECT_EQ(response.getDetections()[0].name, "THREAT");
//     }
//
//     server.requestStop();
//     server.join();
//
//     EXPECT_TRUE(appenderContains("Scanning Server starting listening on socket:"));
//     EXPECT_TRUE(appenderContains("Scanning Server accepting connection:"));
//     EXPECT_TRUE(appenderContains("Scanning Server thread got connection"));
//     EXPECT_TRUE(appenderContains("Closing Scanning Server socket"));
// }
//
// TEST_F(TestThreatDetectorSocket, test_scan_clean) // NOLINT
//{
//     static const std::string THREAT_PATH = "/dev/null";
//     std::string socketPath = "scanning_socket";
//     auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
//     auto scanner = std::make_unique<StrictMock<MockScanner>>();
//
//     auto expected_response = scan_messages::ScanResponse();
//     expected_response.addDetection("/bin/bash", "", "");
//
//     EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
//     EXPECT_CALL(*scannerFactory, createScanner(false, false)).WillOnce(Return(ByMove(std::move(scanner))));
//
//     unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
//     server.start();
//
//     // Create client connection
//     {
//         unixsocket::ScanningClientSocket client_socket(socketPath);
//         TestFile testFile("testfile");
//         auto fd = testFile.open();
//         auto response = scan(client_socket, fd, THREAT_PATH);
//
//         EXPECT_TRUE(response.allClean());
//     }
//
//     server.requestStop();
//     server.join();
// }
//
// TEST_F(TestThreatDetectorSocket, test_scan_twice) // NOLINT
//{
//     static const std::string THREAT_PATH = "/dev/null";
//     std::string socketPath = "scanning_socket";
//     auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
//     auto scanner = std::make_unique<StrictMock<MockScanner>>();
//
//     auto expected_response = scan_messages::ScanResponse();
//     expected_response.addDetection("/bin/bash", "", "");
//
//     EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillRepeatedly(Return(expected_response));
//     EXPECT_CALL(*scannerFactory, createScanner(false, false)).WillOnce(Return(ByMove(std::move(scanner))));
//
//     unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
//     server.start();
//
//     // Create client connection
//     {
//         unixsocket::ScanningClientSocket client_socket(socketPath);
//         TestFile testFile("testfile");
//         auto fd = testFile.open();
//         ASSERT_GE(fd, 0);
//         auto response = scan(client_socket, fd, THREAT_PATH);
//         EXPECT_TRUE(response.allClean());
//
//         fd = testFile.open();
//         ASSERT_GE(fd, 0);
//         response = scan(client_socket, fd, THREAT_PATH);
//         EXPECT_TRUE(response.allClean());
//     }
//
//     server.requestStop();
//     server.join();
// }
//
// TEST_F(TestThreatDetectorSocket, test_scan_throws)
//{
//     static const std::string THREAT_PATH = "/dev/null";
//     std::string socketPath = "scanning_socket";
//     auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
//     auto scanner = std::make_unique<StrictMock<MockScanner>>();
//
//     auto* scannerPtr = scanner.get();
//
//     EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillRepeatedly(Throw(std::runtime_error("Intentional throw")));
//     EXPECT_CALL(*scannerFactory, createScanner(false, false))
//         .WillOnce(Return(ByMove(std::move(scanner))))
//         .WillRepeatedly([](bool, bool) -> threat_scanner::IThreatScannerPtr { return nullptr; });
//
//     unixsocket::ScanningServerSocket server(socketPath, 0600, scannerFactory);
//     server.start();
//
//     // Create client connection
//     {
//         unixsocket::ScanningClientSocket client_socket(socketPath);
//         TestFile testFile("testfile");
//         auto fd = testFile.open();
//         auto response = scan(client_socket, fd, THREAT_PATH);
//         EXPECT_NE(response.getErrorMsg(), "");
//     }
//
//     server.requestStop();
//     server.join();
//
//     testing::Mock::VerifyAndClearExpectations(scannerPtr);
//     testing::Mock::VerifyAndClearExpectations(scannerFactory.get());
//     scannerFactory.reset();
// }
//
// TEST_F(TestThreatDetectorSocket, test_too_many_connections_are_refused)
//{
//     UsingMemoryAppender memoryAppenderHolder(*this);
//
//     static const std::string THREAT_PATH = "/dev/null";
//     std::string socketPath = "/tmp/scanning_socket";
//     auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
//     EXPECT_CALL(*scannerFactory, createScanner(false, false))
//         .WillRepeatedly(
//             [](bool, bool) -> threat_scanner::IThreatScannerPtr
//             { return std::make_unique<StrictMock<MockScanner>>(); });
//
//     unixsocket::ScanningServerSocket server(socketPath, 0600, scannerFactory);
//     server.start();
//
//     // Use a std::list since we can't copy/move ScanningClientSocket
//     std::list<unixsocket::ScanningClientSocket> client_sockets;
//     // Create client connections - more than the max
//     int clientConnectionCount = server.maxClientConnections() + 1;
//     PRINT("Starting " << clientConnectionCount << " client connections");
//
//     ASSERT_GT(clientConnectionCount, 1);
//     for (int i = 0; i < clientConnectionCount; ++i)
//     {
//         auto& socket = client_sockets.emplace_back(socketPath);
//         socket.connect();
//     }
//
//     ASSERT_FALSE(client_sockets.empty());
//     // Can't continue test if we don't have refused connections
//     ASSERT_TRUE(waitForLog("Refusing connection: Maximum number of scanners reached", 500ms));
//
//     // Try a scan with the last connection
//     {
//         assert(!client_sockets.empty()); // make cppcheck happy
//         unixsocket::ScanningClientSocket& client_socket(client_sockets.back());
//         TestFile testFile("testfile");
//         auto fd = testFile.open();
//
//         auto response = scan(client_socket, fd, THREAT_PATH);
//         EXPECT_EQ(response.getErrorMsg(), "Failed to send scan request");
//     }
//
//     server.requestStop();
//     client_sockets.clear();
//     server.join();
// }
