/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <memory>

namespace
{

    class MockIThreatReportCallbacks : public IMessageCallback
    {
    public:
        MOCK_METHOD1(processMessage, void(const std::string& threatDetectedXML));
    };
    class MockScanner : public threat_scanner::IThreatScanner
    {
    public:
        MOCK_METHOD2(scan, scan_messages::ScanResponse(datatypes::AutoFd&, const std::string&));
    };
    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD0(createScanner, threat_scanner::IThreatScannerPtr());
    };
}

using namespace ::testing;

TEST(TestScanningServerSocket, test_construction) //NOLINT
{
    std::string path = "TestThreatDetectorSocket_socket";
    auto mock_callback = std::make_shared<StrictMock<MockIThreatReportCallbacks>>();
    auto scannerFactory
            = std::make_shared<StrictMock<MockScannerFactory> >();
    unixsocket::ScanningServerSocket server(path, mock_callback, scannerFactory);
}
