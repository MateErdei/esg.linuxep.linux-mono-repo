/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"
#include <unixsocket/SocketUtils.h>

#include "tests/common/WaitForEvent.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fcntl.h>
#include <memory>

using namespace ::testing;

namespace
{
    class TestThreatDetectorSocket : public UnixSocketMemoryAppenderUsingTests
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

    class TestFile
    {
    public:
        TestFile(const char* name)
            : m_name(name)
        {
            ::unlink(m_name.c_str());
            datatypes::AutoFd fd(::open(m_name.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR));
        }

        ~TestFile()
        {
            ::unlink(m_name.c_str());
        }

        int open(int oflag = O_RDONLY)
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), oflag));
            return fd.release();
        }

        std::string m_name;
    };
}

TEST_F(TestThreatDetectorSocket, test_construction) //NOLINT
{
    std::string path = "TestThreatDetectorSocket_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_NO_THROW(unixsocket::ScanningServerSocket server(path, 0666, scannerFactory));
}

TEST_F(TestThreatDetectorSocket, test_running) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::string path = "TestThreatDetectorSocket_socket_test_running";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
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
    std::string path = "TestThreatDetectorSocket_socket_test_scan_threat";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/tmp/eicar.com", "THREAT");

    auto* scannerPtr = scanner.get();
    testing::Mock::AllowLeak(scanner.get());

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(path);
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
        auto response = scan(client_socket, fd, THREAT_PATH);

        EXPECT_FALSE(response.allClean());
        EXPECT_EQ(response.getDetections()[0].second, "THREAT");
    }

    server.requestStop();
    server.join();

    testing::Mock::VerifyAndClearExpectations(scannerPtr);
    testing::Mock::VerifyAndClearExpectations(scannerFactory.get());

}

TEST_F(TestThreatDetectorSocket, test_scan_clean) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string path = "TestThreatDetectorSocket_socket_test_scan_clean";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/bin/bash", "");

    auto* scannerPtr = scanner.get();
    testing::Mock::AllowLeak(scanner.get());

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(path);
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
        auto response = scan(client_socket, fd, THREAT_PATH);

        EXPECT_TRUE(response.allClean());
    }

    server.requestStop();
    server.join();

    testing::Mock::VerifyAndClearExpectations(scannerPtr);
    testing::Mock::VerifyAndClearExpectations(scannerFactory.get());

}

TEST_F(TestThreatDetectorSocket, test_scan_twice) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string path = "TestThreatDetectorSocket_socket_test_scan_twice";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.addDetection("/bin/bash", "");

    auto* scannerPtr = scanner.get();
    testing::Mock::AllowLeak(scanner.get());

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillRepeatedly(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(path);
        TestFile testFile("testfile");
        datatypes::AutoFd fd(testFile.open());
        auto response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_TRUE(response.allClean());

        fd.reset(testFile.open());
        response = scan(client_socket, fd, THREAT_PATH);
        EXPECT_TRUE(response.allClean());
    }

    server.requestStop();
    server.join();

    testing::Mock::VerifyAndClearExpectations(scannerPtr);
    testing::Mock::VerifyAndClearExpectations(scannerFactory.get());
}


TEST_F(TestThreatDetectorSocket, test_scan_throws) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string path = "TestThreatDetectorSocket_socket_test_scan_throws";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto* scannerPtr = scanner.get();
    testing::Mock::AllowLeak(scanner.get());

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillRepeatedly(Throw(std::runtime_error("Intentional throw")));
    EXPECT_CALL(*scannerFactory, createScanner(false))
            .WillOnce(Return(ByMove(std::move(scanner))))
            .WillRepeatedly([](bool)->threat_scanner::IThreatScannerPtr{ return nullptr; });

    unixsocket::ScanningServerSocket server(path, 0600, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(path, {0, 10000000});
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




