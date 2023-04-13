// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "MockSusiWrapper.h"
#include "MockSusiWrapperFactory.h"

#include "datatypes/Print.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tests/common/LogInitializedTests.h"
#include "tests/common/MockUpdateCompleteCallback.h"

#define TEST_PUBLIC public

#include "sophos_threat_detector/threat_scanner/ScannerInfo.h"
#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"

using namespace threat_scanner;

namespace{
    class TestSusiScannerFactory : public LogInitializedTests
    {
    };
}

TEST_F(TestSusiScannerFactory, testWithoutPLUGIN_INSTALL)
{
    // SUSI initialization config is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr, nullptr, nullptr);
}

TEST_F(TestSusiScannerFactory, throwsDuringInitializeWithoutPLUGIN_INSTALL)
{
    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr, nullptr, nullptr);

    try
    {
        factory.createScanner(false, false, false);
        FAIL() << "Able to construct scanner without PLUGIN_INSTALL!";
    }
    catch (const std::exception& ex)
    {
        PRINT("Unable to construct instance: " << ex.what() << std::endl);
    }
}

TEST_F(TestSusiScannerFactory, testConstruction)
{
    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr, nullptr, nullptr);
}

TEST_F(TestSusiScannerFactory, testConstructAndShutdown)
{
    SusiScannerFactory factory(nullptr, nullptr, nullptr);
    factory.shutdown();
}

TEST_F(TestSusiScannerFactory, throwsDuringInitialize) //NOLINT
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/opt/not-sophos-spl/plugins/av");

    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr, nullptr, nullptr);

    try
    {
        factory.createScanner(false, false, false);
        FAIL() << "Able to construct scanner!";
    }
    catch (const std::exception& ex)
    {
        PRINT("Unable to construct instance: " << ex.what() << std::endl);
    }
}

TEST_F(TestSusiScannerFactory, testConstructionWithMockSusiWrapper)
{
    ISusiWrapperFactorySharedPtr wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    EXPECT_NO_THROW(SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, nullptr));
}

TEST_F(TestSusiScannerFactory, testCreateScannerWithMockSusiWrapperArchivesAndImagesFalse)
{
    using namespace ::testing;
    auto wrapperFactory = std::make_shared<StrictMock<MockSusiWrapperFactory>>();

    // Rely on TestThreatScanner.cpp to test createScannerInfo
    const auto expectedScannerConfig = "{" + createScannerInfo(false, false, true, true) + "}";

    auto susiWrapper = std::make_shared<StrictMock<MockSusiWrapper>>();
    EXPECT_CALL(*wrapperFactory, isMachineLearningEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(*wrapperFactory, createSusiWrapper(expectedScannerConfig)).WillOnce(Return(susiWrapper));
    EXPECT_CALL(*wrapperFactory, accessGlobalHandler()).WillOnce(Return(nullptr));

    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, nullptr);
    EXPECT_NO_THROW(auto scanner = factory.createScanner(false, false, true));
}


TEST_F(TestSusiScannerFactory, testCreateScannerWithMockSusiWrapperArchivesTrue)
{
    using namespace ::testing;
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();

    // Rely on TestThreatScanner.cpp to test createScannerInfo
    const auto scannerConfig = "{" + createScannerInfo(true, false, true, true) + "}";

    auto susiWrapper = std::make_shared<::testing::StrictMock<MockSusiWrapper>>();
    EXPECT_CALL(*wrapperFactory, isMachineLearningEnabled).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*wrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    EXPECT_CALL(*wrapperFactory, accessGlobalHandler()).WillOnce(Return(nullptr));

    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, nullptr);
    EXPECT_NO_THROW(auto scanner = factory.createScanner(true, false, true));
}


TEST_F(TestSusiScannerFactory, testCreateScannerWithMockSusiWrapperImagesTrue)
{
    using namespace ::testing;
    auto wrapperFactory = std::make_shared<StrictMock<MockSusiWrapperFactory>>();

    // Rely on TestThreatScanner.cpp to test createScannerInfo
    const auto scannerConfig = "{" + createScannerInfo(false, true, true, true) + "}";

    auto susiWrapper = std::make_shared<StrictMock<MockSusiWrapper>>();
    EXPECT_CALL(*wrapperFactory, isMachineLearningEnabled).WillRepeatedly(Return(true));
    EXPECT_CALL(*wrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    EXPECT_CALL(*wrapperFactory, accessGlobalHandler()).WillOnce(Return(nullptr));

    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, nullptr);
    EXPECT_NO_THROW(auto scanner = factory.createScanner(false, true, true));
}

TEST_F(TestSusiScannerFactory, callbackAfterSuccessfulUpdate)
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    auto mockCallback = std::make_shared<::testing::StrictMock<MockUpdateCompleteCallback>>();
    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, mockCallback);

    EXPECT_CALL(*mockCallback, updateComplete()).Times(1);
    EXPECT_CALL(*wrapperFactory, update()).WillOnce(testing::Return(true));

    factory.update();
}


TEST_F(TestSusiScannerFactory, noCallbackAfterSuccessfulUpdate)
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    auto mockCallback = std::make_shared<::testing::StrictMock<MockUpdateCompleteCallback>>();
    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, mockCallback);

    EXPECT_CALL(*wrapperFactory, update()).WillOnce(testing::Return(false));

    factory.update();
}
