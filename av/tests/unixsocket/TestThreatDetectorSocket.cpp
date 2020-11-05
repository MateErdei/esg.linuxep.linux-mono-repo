/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include <unixsocket/SocketUtils.h>
#include "datatypes/sophos_filesystem.h"

#include "tests/common/TestFile.h"
#include "tests/common/WaitForEvent.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fcntl.h>
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

        fs::path m_testDir;
    };

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

        MOCK_METHOD0(update, bool());
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

    EXPECT_TRUE(appenderContains("Starting listening on socket"));
    EXPECT_TRUE(appenderContains("Closing socket"));
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
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "THREAT");

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(socketPath, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(socketPath);
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
        auto response = scan(client_socket, fd, THREAT_PATH);

        EXPECT_FALSE(response.allClean());
        EXPECT_EQ(response.getDetections()[0].second, "THREAT");
    }

    server.requestStop();
    server.join();
}

TEST_F(TestThreatDetectorSocket, test_scan_clean) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string socketPath = "scanning_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/bin/bash", "");

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

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
    expected_response.addDetection("/bin/bash", "");

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillRepeatedly(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

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
    EXPECT_CALL(*scannerFactory, createScanner(false))
            .WillOnce(Return(ByMove(std::move(scanner))))
            .WillRepeatedly([](bool)->threat_scanner::IThreatScannerPtr{ return nullptr; });

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




