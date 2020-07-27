/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <memory>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
        MOCK_METHOD1(createScanner, threat_scanner::IThreatScannerPtr(bool scanArchives));
    };
}

using namespace ::testing;

TEST(TestScanningServerConnectionThread, successful_construction) //NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    EXPECT_NO_THROW(unixsocket::ScanningServerConnectionThread connectionThread(fdHolder, scannerFactory));
}

TEST(TestScanningServerConnectionThread, fail_construction_with_bad_fd) //NOLINT
{
    using namespace unixsocket;
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(fdHolder.get(), -1);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, scannerFactory), std::runtime_error);
}

TEST(TestScanningServerConnectionThread, fail_construction_with_null_factory) //NOLINT
{
    using namespace unixsocket;
    datatypes::AutoFd fdHolder(::open("/dev/null", O_RDONLY));
    ASSERT_GE(fdHolder.get(), 0);
    ASSERT_THROW(ScanningServerConnectionThread(fdHolder, nullptr), std::runtime_error);
}
