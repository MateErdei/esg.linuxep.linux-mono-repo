// Copyright 2022, Sophos Limited. All rights reserved.

#include "sophos_threat_detector/threat_scanner/SusiScanResultJsonParser.h"

#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>

using namespace threat_scanner;
using namespace ::testing;

class TestSusiScanResultJsonParser : public LogOffInitializedTests
{
};

TEST_F(TestSusiScanResultJsonParser, SusiManualExample_NoError)
{
    // This is the example given in the SUSI manual.
    // However, current version of SUSI wouldn't give the sha256 for the top-level stream as it has no detection

    ScanResult result = parseSusiScanResultJson(R"({
    "time": "2019-03-04T02:55Z",
    "results": [
        {
            "base64path": "L3Vzci9iaW4vbWFsd2FyZS56aXAK",
            "path": "/usr/bin/malware.zip",
            "sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
            "TFTclassification": {
                "fileType": "TFT/PK-C",
                "fileTypeDescription": "Zip Archive",
                "typeId": "ZIP"
            }
        },
        {
            "base64path": "L3Vzci9iaW4vbWFsd2FyZS56aXAvZmlsZTEuZXhlCg==",
            "path": "/usr/bin/malware.zip/file1.exe",
            "sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
            "detections": [
                {
                    "threatName": "ML/PE-A",
                    "threatType": "virus"
                },
                {
                    "threatName": "MAL/malware-A",
                    "threatType": "virus"
                }
            ],
            "TFTclassification": {
                "fileType": "TFT/PE-A",
                "fileTypeDescription": "PE executable",
                "typeId": "PE"
            }
        },
        {
            "base64path": "L3Vzci9iaW4vbWFsd2FyZS56aXAvZmlsZTIuZXhlCg==",
            "path": "/usr/bin/malware.zip/file2.exe",
            "sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
            "detections": [
                {
                    "threatName": "Troj/Generic-A",
                    "threatType": "trojan"
                }
            ],
            "TFTclassification": {
                "fileType": "TFT/PE-A",
                "fileTypeDescription": "PE executable",
                "typeId": "Windows"
            },
            "submitToAnalysis": {
                "identity": "Sand/Gen-a",
                "revision": "42"
            }
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 2);
    EXPECT_EQ(
        result.detections.at(0),
        (Detection{ "/usr/bin/malware.zip/file1.exe\n", // The example actually includes \n in the base64path
                    "ML/PE-A",
                    "virus",
                    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" }));
    EXPECT_EQ(
        result.detections.at(1),
        (Detection{ "/usr/bin/malware.zip/file2.exe\n",
                    "Troj/Generic-A",
                    "trojan",
                    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" }));
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestSusiScanResultJsonParser, MinimalValidDetection_NoError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/tmp/eicar.txt", "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestSusiScanResultJsonParser, MinimalValidScanError_ScanError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "error": "encrypted"
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_EQ(result.errors.at(0).message, "Failed to scan /tmp/eicar.txt as it is password protected");
}

TEST_F(TestSusiScanResultJsonParser, ValidResultWithScanError_DetectionAndError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ],
            "error": "encrypted"
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/tmp/eicar.txt", "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_EQ(result.errors.at(0).message, "Failed to scan /tmp/eicar.txt as it is password protected");
}

TEST_F(TestSusiScanResultJsonParser, MultipleValidResultWithScanError_DetectionsAndErrors)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3Vzci9iaW4vbWFsd2FyZS56aXAvZmlsZTEuZXhl",
            "sha256": "sha2561",
            "detections": [
                {
                    "threatName": "name1",
                    "threatType": "type1"
                }
            ],
            "error": "encrypted"
        },
        {
            "base64path": "L3Vzci9iaW4vbWFsd2FyZS56aXAvZmlsZTIuZXhl",
            "sha256": "sha2562",
            "detections": [
                {
                    "threatName": "name2",
                    "threatType": "type2"
                }
            ],
            "error": "corrupt"
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 2);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/usr/bin/malware.zip/file1.exe", "name1", "type1", "sha2561" }));
    EXPECT_EQ(result.detections.at(1), (Detection{ "/usr/bin/malware.zip/file2.exe", "name2", "type2", "sha2562" }));
    ASSERT_EQ(result.errors.size(), 2);
    EXPECT_EQ(result.errors.at(0).message, "Failed to scan /usr/bin/malware.zip/file1.exe as it is password protected");
    EXPECT_EQ(result.errors.at(1).message, "Failed to scan /usr/bin/malware.zip/file2.exe as it is corrupted");
}

TEST_F(TestSusiScanResultJsonParser, DuplicateValidResults_SeparateDetections)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        },
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 2);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/tmp/eicar.txt", "name", "type", "sha256" }));
    EXPECT_EQ(result.detections.at(1), (Detection{ "/tmp/eicar.txt", "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestSusiScanResultJsonParser, InvalidJson_Error)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": )");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, EmptyObject_Error)
{
    ScanResult result = parseSusiScanResultJson("{}");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, EmptyResults_NoError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": []
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestSusiScanResultJsonParser, EmptyResult_ErrorNoDetections)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {}
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, ResultWithOnlyBase64Path_NoError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ="
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestSusiScanResultJsonParser, ResultWithDetectionWithoutBase64Path_Error)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, ResultWithDetectionWithoutSha256_Error)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, ResultWithDetectionWithoutName_NoError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, ResultWithDetectionWithoutType_NoError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, ArrayNotObjectJson_Error)
{
    ScanResult result = parseSusiScanResultJson(R"([
    {
        "base64path": "L3RtcC9laWNhci50eHQ=",
        "sha256": "sha256",
        "detections": [
            {
                "threatName": "name",
                "threatType": "type"
            }
        ]
    }
])");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, ResultsIsObject_Error)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": {
        "base64path": "L3RtcC9laWNhci50eHQ=",
        "sha256": "sha256",
        "detections": [
            {
                "threatName": "name",
                "threatType": "type"
            }
        ]
    }
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, InvalidResultWithScanError_OnlyParseError)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "detections": [
                {}
            ],
            "error": "encrypted"
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, FirstInvalidDetectionSecondValidDetection_ErrorAndNoDetectionb)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {},
        {
            "base64path": "L3RtcC9laWNhci50eHQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, InvalidBase64Path)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "!",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_THAT(result.errors.at(0).message, StartsWith("Failed to parse SUSI response"));
}

TEST_F(TestSusiScanResultJsonParser, WeirdBase64Path)
{
    ScanResult result = parseSusiScanResultJson(R"({
    "results": [
        {
            "base64path": "AAEC",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ]
        }
    ]
})");
    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(
        result.detections.at(0), (Detection{ std::string({ '\x00', '\x01', '\x02' }), "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 0);
}