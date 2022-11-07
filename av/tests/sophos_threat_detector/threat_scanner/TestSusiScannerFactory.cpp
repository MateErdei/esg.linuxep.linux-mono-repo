// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "MockSusiWrapper.h"
#include "MockSusiWrapperFactory.h"

#include "datatypes/Print.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../../common/LogInitializedTests.h"

#define TEST_PUBLIC public

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
        factory.createScanner(false, false);
        FAIL() << "Able to construct scanner without PLUGIN_INSTALL!";
    }
    catch (const std::exception& ex)
    {
        PRINT("Unable to construct instance: " << ex.what() << std::endl);
    }
}

TEST_F(TestSusiScannerFactory, testConstruction)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/opt/not-sophos-spl/plugins/av");

    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr, nullptr, nullptr);
}

TEST_F(TestSusiScannerFactory, throwsDuringInitialize) //NOLINT
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/opt/not-sophos-spl/plugins/av");

    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr, nullptr, nullptr);

    try
    {
        factory.createScanner(false, false);
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
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, nullptr);

    const std::string scannerConfig = R"sophos({"scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": false,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": false,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": false
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    }})sophos";

    auto susiWrapper = std::make_shared<::testing::StrictMock<MockSusiWrapper>>();
    EXPECT_CALL(*wrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    EXPECT_NO_THROW(auto scanner = factory.createScanner(false, false));
}


TEST_F(TestSusiScannerFactory, testCreateScannerWithMockSusiWrapperArchivesTrue)
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, nullptr);

    const std::string scannerConfig = R"sophos({"scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": true,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": true,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": false
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    }})sophos";

    auto susiWrapper = std::make_shared<::testing::StrictMock<MockSusiWrapper>>();
    EXPECT_CALL(*wrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    EXPECT_NO_THROW(auto scanner = factory.createScanner(true, false));
}


TEST_F(TestSusiScannerFactory, testCreateScannerWithMockSusiWrapperImagesTrue)
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, nullptr);

    const std::string scannerConfig = R"sophos({"scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": false,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": false,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": true
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    }})sophos";

    auto susiWrapper = std::make_shared<::testing::StrictMock<MockSusiWrapper>>();
    EXPECT_CALL(*wrapperFactory, createSusiWrapper(scannerConfig)).WillOnce(Return(susiWrapper));
    EXPECT_NO_THROW(auto scanner = factory.createScanner(false, true));
}

namespace
{
    class MockUpdateCallback : public threat_scanner::IUpdateCompleteCallback
    {
    public:
        MOCK_METHOD(void, updateComplete, (), (override));
    };
}

TEST_F(TestSusiScannerFactory, callbackAfterSuccessfulUpdate)
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    auto mockCallback = std::make_shared<::testing::StrictMock<MockUpdateCallback>>();
    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, mockCallback);

    EXPECT_CALL(*mockCallback, updateComplete()).Times(1);
    EXPECT_CALL(*wrapperFactory, update()).WillOnce(testing::Return(true));

    factory.update();
}


TEST_F(TestSusiScannerFactory, noCallbackAfterSuccessfulUpdate)
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    auto mockCallback = std::make_shared<::testing::StrictMock<MockUpdateCallback>>();
    SusiScannerFactory factory(wrapperFactory, nullptr, nullptr, mockCallback);

    EXPECT_CALL(*wrapperFactory, update()).WillOnce(testing::Return(false));

    factory.update();
}
