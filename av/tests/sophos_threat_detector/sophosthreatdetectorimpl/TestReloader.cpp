// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/sophosthreatdetectorimpl/Reloader.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatDetectorException.h"

#include "tests/common/LogInitializedTests.h"
#include "tests/common/MockScanner.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace
{
    class TestReloader : public LogInitializedTests
    {
    };
}


TEST_F(TestReloader, testNoArgConstruction)
{
    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::Reloader reloader);
}

TEST_F(TestReloader, testScannerFactoryConstruction)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_NO_THROW(sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory));
}

TEST_F(TestReloader, testReload)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, reload()).WillOnce(Return(true));
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_NO_THROW(reloader.reload());
}

TEST_F(TestReloader, testReloadFails)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, reload()).WillOnce(Return(false));
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_THROW(reloader.reload(), sspl::sophosthreatdetectorimpl::ThreatDetectorException);
}

TEST_F(TestReloader, testReloadWithoutFactoryThrows)
{
    sspl::sophosthreatdetectorimpl::Reloader reloader;
    EXPECT_THROW(reloader.reload(), sspl::sophosthreatdetectorimpl::ThreatDetectorException);
}

TEST_F(TestReloader, testUpdate)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(true));
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_NO_THROW(reloader.update());
}

TEST_F(TestReloader, testUpdateFails)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(false));
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    EXPECT_THROW(reloader.update(), sspl::sophosthreatdetectorimpl::ThreatDetectorException);
}

TEST_F(TestReloader, testUpdateWithoutFactoryThrows)
{
    sspl::sophosthreatdetectorimpl::Reloader reloader;
    EXPECT_THROW(reloader.update(), sspl::sophosthreatdetectorimpl::ThreatDetectorException);
}


TEST_F(TestReloader, testReset)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory, update()).WillOnce(Return(true));
    sspl::sophosthreatdetectorimpl::Reloader reloader;
    reloader.reset(scannerFactory);
    EXPECT_NO_THROW(reloader.update());
}

TEST_F(TestReloader, testResetWithExistingFactory)
{
    auto scannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    auto scannerFactory2 = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*scannerFactory2, update()).WillOnce(Return(true));
    sspl::sophosthreatdetectorimpl::Reloader reloader(scannerFactory);
    reloader.reset(scannerFactory2);
    EXPECT_NO_THROW(reloader.update());
}
