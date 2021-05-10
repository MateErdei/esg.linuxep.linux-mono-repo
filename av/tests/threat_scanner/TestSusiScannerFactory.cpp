/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockSusiWrapper.h"
#include "MockSusiWrapperFactory.h"

#include "datatypes/Print.h"
#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace threat_scanner;

TEST(TestSusiScannerFactory, testWithoutPLUGIN_INSTALL) // NOLINT
{
    // SUSI initialization config is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr);
}

TEST(TestSusiScannerFactory, throwsDuringInitializeWithoutPLUGIN_INSTALL) // NOLINT
{
    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr);

    try
    {
        factory.createScanner(false);
        FAIL() << "Able to construct scanner without PLUGIN_INSTALL!";
    }
    catch (const std::exception& ex)
    {
        PRINT("Unable to construct instance: " << ex.what() << '\n');
    }
}

TEST(TestSusiScannerFactory, testConstruction) // NOLINT
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/opt/not-sophos-spl/plugins/av");

    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr);
}

TEST(TestSusiScannerFactory, throwsDuringInitialize) //NOLINT
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/opt/not-sophos-spl/plugins/av");

    // SUSI initialization is now deferred, so constructor won't fail.
    SusiScannerFactory factory(nullptr);

    try
    {
        factory.createScanner(false);
        FAIL() << "Able to construct scanner!";
    }
    catch (const std::exception& ex)
    {
        PRINT("Unable to construct instance: " << ex.what() << '\n');
    }
}

TEST(TestSusiScannerFactory, testConstructionWithMockSusiWrapper) // NOLINT
{
    ISusiWrapperFactorySharedPtr wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    EXPECT_NO_THROW(SusiScannerFactory factory(wrapperFactory, nullptr));
}

TEST(TestSusiScannerFactory, testCreateScannerWithMockSusiWrapperArchivesFalse) // NOLINT
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    SusiScannerFactory factory(wrapperFactory, nullptr);

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

    EXPECT_NO_THROW(auto scanner = factory.createScanner(false));
}


TEST(TestSusiScannerFactory, testCreateScannerWithMockSusiWrapperArchivesTrue) // NOLINT
{
    auto wrapperFactory = std::make_shared<::testing::StrictMock<MockSusiWrapperFactory>>();
    SusiScannerFactory factory(wrapperFactory, nullptr);

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

    EXPECT_NO_THROW(auto scanner = factory.createScanner(true));
}

