/******************************************************************************************************

Copyright 2021-2022, Sophos Limited.  All rights reserved.

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
        MOCK_METHOD0(reload, ReloadResult());
        MOCK_METHOD0(shutdown, void());
        MOCK_METHOD0(susiIsInitialized, bool());
        MOCK_METHOD(bool, hasConfigChanged, ());
    };
}



TEST_F(TestReloader, testNoArgConstruction) // NOLINT
{
    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::Reloader reloader);
}

TEST_F(TestReloader, testScannerFactoryConstruction) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto rescanWorker = std::make_shared<SafeStoreRescanWorker>("/var/safestore_rescan_socket");
    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory,rescanWorker));
}

TEST_F(TestReloader, testReload) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, reload()).WillOnce(Return(ReloadResult{true,true}));

    auto rescanWorker = std::make_shared<SafeStoreRescanWorker>("/var/safestore_rescan_socket");
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory,rescanWorker);
    EXPECT_NO_THROW(reloader.reload());
}

TEST_F(TestReloader, testReloadFails) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, reload()).WillOnce(Return(ReloadResult{false,false}));

    auto rescanWorker = std::make_shared<SafeStoreRescanWorker>("/var/safestore_rescan_socket");
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory,rescanWorker);
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

    auto rescanWorker = std::make_shared<SafeStoreRescanWorker>("/var/safestore_rescan_socket");
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory,rescanWorker);
    EXPECT_NO_THROW(reloader.update());
}

TEST_F(TestReloader, testUpdateFails) // NOLINT
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(false));

    auto rescanWorker = std::make_shared<SafeStoreRescanWorker>("/var/safestore_rescan_socket");
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory,rescanWorker);
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
    auto rescanWorker = std::make_shared<SafeStoreRescanWorker>("/var/safestore_rescan_socket");
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory,rescanWorker);
    reloader.reset(scannerFactory2);
    EXPECT_NO_THROW(reloader.update());
}
