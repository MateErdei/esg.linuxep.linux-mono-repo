/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <memory>

static Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;

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
        MOCK_METHOD0(createScanner, threat_scanner::IThreatScannerPtr());
    };
}

using namespace ::testing;

TEST(TestScanningServerConnectionThread, successful_construction) //NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_NO_THROW(unixsocket::ScanningServerConnectionThread connectionThread(2, scannerFactory));
}

TEST(TestScanningServerConnectionThread, fail_construction_with_bad_fd) //NOLINT
{
    using namespace unixsocket;
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    ASSERT_THROW(ScanningServerConnectionThread(-1, scannerFactory), std::runtime_error);
}

TEST(TestScanningServerConnectionThread, fail_construction_with_null_factory) //NOLINT
{
    using namespace unixsocket;
    ASSERT_THROW(ScanningServerConnectionThread(2, nullptr), std::runtime_error);
}
