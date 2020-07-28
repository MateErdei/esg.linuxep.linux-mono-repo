/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "tests/common/WaitForEvent.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unixsocket/SocketUtils.h>

#include <fcntl.h>
#include <memory>

namespace
{

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

TEST(TestThreatDetectorSocket, test_construction) //NOLINT
{
    std::string path = "TestThreatDetectorSocket_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_NO_THROW(unixsocket::ScanningServerSocket server(path, 0666, scannerFactory));
}

TEST(TestThreatDetectorSocket, test_running) // NOLINT
{
    std::string path = "TestThreatDetectorSocket_socket_test_running";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.start();
    server.requestStop();
    server.join();
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

TEST(TestThreatDetectorSocket, test_scan) // NOLINT
{
    static const std::string THREAT_PATH = "/dev/null";
    std::string path = "TestThreatDetectorSocket_socket_test_running";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scanner = std::make_unique<StrictMock<MockScanner>>();

    auto expected_response = scan_messages::ScanResponse();
    expected_response.setClean(false);
    expected_response.setThreatName("THREAT");

    auto* scannerPtr = scanner.get();
    testing::Mock::AllowLeak(scanner.get());

    EXPECT_CALL(*scanner, scan(_, THREAT_PATH, _, _)).WillOnce(Return(expected_response));
    EXPECT_CALL(*scannerFactory, createScanner(false)).WillOnce(Return(ByMove(std::move(scanner))));

    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.start();

    // Create client connection
    {
        unixsocket::ScanningClientSocket client_socket(path);
        datatypes::AutoFd devNull(::open(THREAT_PATH.c_str(), O_RDONLY));
        auto response = scan(client_socket, devNull, THREAT_PATH);

        EXPECT_FALSE(response.clean());
        EXPECT_EQ(response.threatName(), "THREAT");
    }

    server.requestStop();
    server.join();

    testing::Mock::VerifyAndClearExpectations(scannerPtr);
    testing::Mock::VerifyAndClearExpectations(scannerFactory.get());

}

TEST(TestSocketUtils, environmentInterruptionReportsWhat) // NOLINT
{
    try
    {
        throw unixsocket::environmentInterruption();
    }
    catch (const unixsocket::environmentInterruption& ex)
    {
        EXPECT_EQ(ex.what(), "Environment interruption");
    }
}
