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

static std::string susiResponseStr =
        "{\n"
        "    \"results\":\n"
        "    [\n"
        "        {\n"
        "            \"detections\": [\n"
        "               {\n"
        "                   \"threatName\": \"MAL/malware-A\",\n"
        "                   \"threatType\": \"malware/trojan/PUA/...\",\n"
        "                   \"threatPath\": \"...\",\n"
        "                   \"longName\": \"...\",\n"
        "                   \"identityType\": 0,\n"
        "                   \"identitySubtype\": 0\n"
        "               }\n"
        "            ],\n"
        "            \"Local\": {\n"
        "                \"LookupType\": 1,\n"
        "                \"Score\": 20,\n"
        "                \"SignerStrength\": 30\n"
        "            },\n"
        "            \"Global\": {\n"
        "                \"LookupType\": 1,\n"
        "                \"Score\": 40\n"
        "            },\n"
        "            \"ML\": {\n"
        "                \"ml-pe\": 50\n"
        "            },\n"
        "            \"properties\": {\n"
        "                \"GENES/SUPPRESSML\": 1\n"
        "            },\n"
        "            \"telemetry\": [\n"
        "                {\n"
        "                    \"identityName\": \"xxx\",\n"
        "                    \"dataHex\": \"6865646765686f67\"\n"
        "                }\n"
        "            ],\n"
        "            \"mlscores\": [\n"
        "                {\n"
        "                    \"score\": 12,\n"
        "                    \"secScore\": 34,\n"
        "                    \"featuresHex\": \"6865646765686f67\",\n"
        "                    \"mlDataVersion\": 56\n"
        "                }\n"
        "            ]\n"
        "        }\n"
        "    ]\n"
        "}";

TEST(TestThreatScanner, test_FakeSusiScannerConstruction) //NOLINT
{
    threat_scanner::FakeSusiScannerFactory factory;
    auto scanner = factory.createScanner(false);
    scanner.reset();
}

TEST(TestThreatScanner, test_SusiScannerConstruction) //NOLINT
{
    setupFakeSophosThreatDetectorConfig();

    fs::path libraryPath = pluginInstall() / "chroot/susi/distribution_version";

    static const std::string scannerInfo = R"("scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": false,
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

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false);
}

TEST(TestThreatScanner, test_SusiScannerConstructionWithScanArchives) //NOLINT
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

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, true);
}

TEST(TestThreatScanner, test_SusiScanner_scanFile_clean) //NOLINT
{
    setupFakeSophosThreatDetectorConfig();

    auto susiWrapper = std::make_shared<MockSusiWrapper>("", "");
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_, _)).WillOnce(Return(susiWrapper));

    SusiResult susiResult = SUSI_S_OK;
    SusiScanResult* scanResult = nullptr;
    std::string filePath = "/tmp/clean_file.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(Return(susiResult));
    EXPECT_CALL(*susiWrapper, freeResult(scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false);
    datatypes::AutoFd fd(1);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath);

    EXPECT_EQ(response.clean(), true);
}

TEST(TestThreatScanner, test_SusiScanner_scanFile_threat) //NOLINT
{
    setupFakeSophosThreatDetectorConfig();

    auto susiWrapper = std::make_shared<MockSusiWrapper>("", "");
    std::shared_ptr<MockSusiWrapperFactory> susiWrapperFactory = std::make_shared<MockSusiWrapperFactory>();

    EXPECT_CALL(*susiWrapperFactory, createSusiWrapper(_, _)).WillOnce(Return(susiWrapper));

    SusiResult susiResult = SUSI_I_THREATPRESENT;
    SusiScanResult scanResult;
    scanResult.version = 1;
    scanResult.scanResultJson = const_cast<char*>(susiResponseStr.c_str());
    std::string filePath = "/tmp/eicar.txt";

    EXPECT_CALL(*susiWrapper, scanFile(_, filePath.c_str(), _, _)).WillOnce(DoAll(SetArgPointee<3>(&scanResult), Return(susiResult)));
    EXPECT_CALL(*susiWrapper, freeResult(&scanResult));

    threat_scanner::SusiScanner susiScanner(susiWrapperFactory, false);
    datatypes::AutoFd fd(1);
    scan_messages::ScanResponse response = susiScanner.scan(fd, filePath);

    EXPECT_EQ(response.clean(), false);
}