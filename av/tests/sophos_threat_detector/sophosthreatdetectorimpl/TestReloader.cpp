/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_threat_detector/sophosthreatdetectorimpl/Reloader.h"
#include "../../common/LogInitializedTests.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace
{
    class TestReloader : public LogInitializedTests
    {
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



TEST_F(TestReloader, testNoArgConstruction) // NOLINT
{
    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::Reloader reloader);
}

TEST_F(TestReloader, testScannerFactoryConstruction) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory));
}

TEST_F(TestReloader, testReload) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, reload()).WillOnce(Return(true));

    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_NO_THROW(reloader.reload());
}

TEST_F(TestReloader, testReloadFails) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, reload()).WillOnce(Return(false));

    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_THROW(reloader.reload(), std::runtime_error);
}

TEST_F(TestReloader, testReloadWithoutFactoryThrows) // NOLINT
{
    sspl::sophosthreatdetectorimpl::Reloader reloader;
    EXPECT_THROW(reloader.reload(), std::runtime_error);
}

TEST_F(TestReloader, testUpdate) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(true));

    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_NO_THROW(reloader.update());
}

TEST_F(TestReloader, testUpdateFails) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(false));

    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_THROW(reloader.update(), std::runtime_error);
}

TEST_F(TestReloader, testUpdateWithoutFactoryThrows) // NOLINT
{
    sspl::sophosthreatdetectorimpl::Reloader reloader;
    EXPECT_THROW(reloader.update(), std::runtime_error);
}


TEST_F(TestReloader, testReset) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(true));

    sspl::sophosthreatdetectorimpl::Reloader reloader;
    reloader.reset(scannerFactory);
    EXPECT_NO_THROW(reloader.update());
}

TEST_F(TestReloader, testResetWithExistingFactory) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    auto scannerFactory2 = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory2, update()).WillOnce(Return(true));

    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    reloader.reset(scannerFactory2);
    EXPECT_NO_THROW(reloader.update());
}
