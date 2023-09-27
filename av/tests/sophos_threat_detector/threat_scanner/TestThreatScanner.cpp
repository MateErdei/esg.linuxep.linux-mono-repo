// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/threat_scanner/ScannerInfo.h"
#include "sophos_threat_detector/threat_scanner/ThrowIfNotOk.h"
#include "sophos_threat_detector/threat_scanner/ThreatScannerException.h"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

#include <string>

using namespace threat_scanner;

TEST(TestScannerInfo, test_SusiScannerConstruction)
{
    const std::string scannerInfo = R"("scanner": {
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
                "puaDetection": true,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        },
        "nextGen": {
            "scanControl": {
                "machineLearning": false
            }
        }
    })";

    EXPECT_EQ(createScannerInfo(false, false, true, false), scannerInfo);
}

TEST(TestScannerInfo, test_SusiScannerConstructionWithScanArchives)
{
    const std::string scannerInfo = R"("scanner": {
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
                "puaDetection": true,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        },
        "nextGen": {
            "scanControl": {
                "machineLearning": false
            }
        }
    })";

    EXPECT_EQ(createScannerInfo(true, false, true, false), scannerInfo);
}

TEST(TestScannerInfo, test_SusiScannerConstructionWithScanImages)
{
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
                "webArchive": false,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": true
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": true,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        },
        "nextGen": {
            "scanControl": {
                "machineLearning": false
            }
        }
    })";

    EXPECT_EQ(createScannerInfo(false, true, true, false), scannerInfo);
}

TEST(TestScannerInfo, machineLearningTrue)
{
    const auto actual = createScannerInfo(false, true, true, true);

    using namespace nlohmann;
    json parsed = json::parse("{" + actual + "}");
    EXPECT_EQ(parsed.at("scanner").at("nextGen").at("scanControl").at("machineLearning"), true);
}

TEST(TestScannerInfo, machineLearningFalse)
{
    const auto actual = createScannerInfo(false, true, true, false);

    using namespace nlohmann;
    json parsed = json::parse("{" + actual + "}");
    EXPECT_EQ(parsed.at("scanner").at("nextGen").at("scanControl").at("machineLearning"), false);
}

TEST(TestThrowIfNotOk, TestOk)
{
    EXPECT_NO_THROW(throwIfNotOk(SUSI_S_OK, "Should not throw"));
}

TEST(TestThrowIfNotOk, TestNotOk)
{
    EXPECT_THROW(throwIfNotOk(SUSI_E_BAD_JSON, "Should throw"), ThreatScannerException);
}
