/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MockSusiWrapper.h"
#include "MockSusiWrapperFactory.h"

#include "sophos_threat_detector/threat_scanner/FakeSusiScannerFactory.h"
#include "sophos_threat_detector/threat_scanner/SusiScanner.h"
#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"
#include "../common/Common.h"

#include "Common/UtilityImpl/StringUtils.h"

using namespace testing;

TEST(TestThreatScanner, test_FakeSusiScannerConstruction) //NOLINT
{
    threat_scanner::FakeSusiScannerFactory factory;
    auto scanner = factory.createScanner();
    scanner.reset();
}

TEST(TestThreatScanner, test_SusiScannerConstruction) //NOLINT
{
    setupFakeSophosThreatDetectorConfig();

    fs::path libraryPath = pluginInstall() / "chroot/susi/distribution_version";

    static const std::string scannerInfo = R"("scanner": {
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
                "macintosh": true
            },
            "scanControl": {
                "trueFileTypeDetection": true,
                "puaDetection": true,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    })";

    std::string runtimeConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(R"sophos({
    "library": {
        "libraryPath": "@@LIBRARY_PATH@@",
        "tempPath": "/tmp",
        "product": {
            "name": "SSPL AV Plugin",
            "context": "File",
            "version": "1.0.0"
        },
        "customerID": "0123456789abcdef",
        "machineID": "fedcba9876543210"
    },
    @@SCANNER_CONFIG@@
})sophos", {{"@@LIBRARY_PATH@@", libraryPath},
            {"@@SCANNER_CONFIG@@", scannerInfo}
    });

    std::string scannerConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(R"sophos({
        @@SCANNER_CONFIG@@
})sophos", {{"@@SCANNER_CONFIG@@", scannerInfo}
    });

    auto susiWrapper = std::make_shared<MockSusiWrapper>(runtimeConfig, scannerConfig);
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(susiWrapper->m_runtimeConfig,
            susiWrapper->m_scannerConfig)).WillOnce(Return(susiWrapper));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory);
}