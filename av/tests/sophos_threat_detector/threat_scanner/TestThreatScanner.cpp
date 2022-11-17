// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "sophos_threat_detector/threat_scanner/ScannerInfo.h"
#include "sophos_threat_detector/threat_scanner/ThrowIfNotOk.h"

#include <gtest/gtest.h>

#include <SusiTypes.h>
#include <string>

using namespace threat_scanner;

TEST(TestThreatScanner, test_SusiScannerConstruction)
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
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    })";

    EXPECT_EQ(create_scanner_info(false, false), scannerInfo);
}

TEST(TestThreatScanner, test_SusiScannerConstructionWithScanArchives)
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
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    })";

    EXPECT_EQ(create_scanner_info(true, false), scannerInfo);
}

TEST(TestThreatScanner, test_SusiScannerConstructionWithScanImages)
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
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        }
    })";

    EXPECT_EQ(create_scanner_info(false, true), scannerInfo);
}

TEST(TestThrowIfNotOk, TestOk)
{
    EXPECT_NO_THROW(throwIfNotOk(SUSI_S_OK, "Should not throw"));
}

TEST(TestThrowIfNotOk, TestNotOk)
{
    EXPECT_THROW(throwIfNotOk(SUSI_E_BAD_JSON, "Should throw"), std::runtime_error);
}