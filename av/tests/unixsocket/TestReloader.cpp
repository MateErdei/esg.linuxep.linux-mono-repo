/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UnixSocketMemoryAppenderUsingTests.h"

#include "unixsocket/threatDetectorSocket/Reloader.h"


#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

namespace
{
    class TestReloader : public UnixSocketMemoryAppenderUsingTests
    {
    };

    class MockScannerFactory : public threat_scanner::IThreatScannerFactory
    {
    public:
        MOCK_METHOD1(createScanner, threat_scanner::IThreatScannerPtr(bool scanArchives));

        MOCK_METHOD0(update, bool());
    };
}



TEST_F(TestReloader, testNoArgConstruction) // NOLINT
{
    EXPECT_NO_THROW(unixsocket::Reloader reloader);
}

TEST_F(TestReloader, testScannerFactoryConstruction) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    EXPECT_NO_THROW(unixsocket::Reloader reloader(scannerFactory));
}

TEST_F(TestReloader, testReload) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(true));

    unixsocket::Reloader reloader(scannerFactory);
    EXPECT_NO_THROW(reloader.reload());
}

TEST_F(TestReloader, testReloadFails) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(false));

    unixsocket::Reloader reloader(scannerFactory);
    EXPECT_THROW(reloader.reload(), std::runtime_error);
}

TEST_F(TestReloader, testReloadWithoutFactoryThrows) // NOLINT
{
    unixsocket::Reloader reloader;
    EXPECT_THROW(reloader.reload(), std::runtime_error);
}

TEST_F(TestReloader, testReset) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(true));

    unixsocket::Reloader reloader;
    reloader.reset(scannerFactory);
    EXPECT_NO_THROW(reloader.reload());
}

TEST_F(TestReloader, testResetWithExistingFactory) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    auto scannerFactory2 = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory2, update()).WillOnce(Return(true));

    unixsocket::Reloader reloader(scannerFactory);
    reloader.reset(scannerFactory2);
    EXPECT_NO_THROW(reloader.reload());
}
