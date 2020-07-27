/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "tests/common/WaitForEvent.h"

#include <memory>

namespace
{

    class MockScanner : public threat_scanner::IThreatScanner
    {
    public:
        MOCK_METHOD2(scan, scan_messages::ScanResponse(datatypes::AutoFd&, const std::string&));
    };
    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD1(createScanner, threat_scanner::IThreatScannerPtr(bool scanArchives));
    };
}

using namespace ::testing;

TEST(TestScanningServerSocket, test_construction) //NOLINT
{
    std::string path = "TestThreatDetectorSocket_socket";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_NO_THROW(unixsocket::ScanningServerSocket server(path, 0666, scannerFactory));
}

TEST(TestScanningServerSocket, test_running) // NOLINT
{
    std::string path = "TestThreatDetectorSocket_socket_test_running";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.start();
    server.requestStop();
    server.join();
}

TEST(TestScanningServerSocket, test_connect) // NOLINT
{
    WaitForEvent serverWaitGuard;
    std::string path = "TestThreatDetectorSocket_socket_test_running";
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.start();

    // Create client connection

    server.requestStop();
    server.join();
}

