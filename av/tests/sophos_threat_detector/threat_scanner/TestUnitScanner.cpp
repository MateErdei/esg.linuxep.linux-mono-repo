// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MockSusiWrapper.h"

#include "sophos_threat_detector/threat_scanner/SusiLogger.h"
#include "sophos_threat_detector/threat_scanner/UnitScanner.h"

#include "tests/common/MemoryAppender.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace threat_scanner;

namespace
{
    class TestUnitScanner : public MemoryAppenderUsingTests
    {
    protected:
        TestUnitScanner() : MemoryAppenderUsingTests("ThreatScanner"), usingMemoryAppender_{ *this }
        {
        }

        UsingMemoryAppender usingMemoryAppender_;
    };
} // namespace

TEST_F(TestUnitScanner, NoDetectionSusiOk)
{
    const std::string path = "/tmp/clean_file.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": []
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_S_OK)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestUnitScanner, NoDetection_SusiReturnCodeError__AddsError)
{
    const std::string path = "/tmp/clean_file.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": []
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_E_OUTOFMEMORY)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_EQ(result.errors.at(0).message, "Failed to scan /tmp/clean_file.txt due to a susi out of memory error");
}

TEST_F(TestUnitScanner, NoDetection_SusiReturnCodeThreat__CreatesDummyDetectionCalculatesSha256AndAddsError)
{
    const std::string path = "/tmp/clean_file.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": []
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_I_THREATPRESENT)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, -1)).Times(1).WillOnce(Return("calculatedSha256"));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ path, "unknown", "unknown", "calculatedSha256" }));
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_EQ(result.errors.at(0).message, "SUSI detected a threat but didn't provide any detections");
}

TEST_F(TestUnitScanner, NoDetection_SusiReturnCodeThreat_Sha256Fails_CreatesDummyDetectionWithUnknownSha256AndAddsError)
{
    const std::string path = "/tmp/clean_file.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": []
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_I_THREATPRESENT)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, -1)).Times(1).WillOnce(Throw(std::runtime_error("message")));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ path, "unknown", "unknown", "unknown" }));
    ASSERT_EQ(result.errors.size(), 2);
    EXPECT_EQ(result.errors.at(0).message, "SUSI detected a threat but didn't provide any detections");
    EXPECT_EQ(result.errors.at(1).message, "Failed to calculate the SHA256 of /tmp/clean_file.txt: message");
}

TEST_F(TestUnitScanner, Detection_SusiReturnCodeThreat__IsThreatIsTrue)
{
    const std::string path = "/tmp/eicar.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": [
        {
            "base64path": "L3BhdGgvZnJvbS9iYXNlNjQ=",
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

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_I_THREATPRESENT)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/path/from/base64", "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestUnitScanner, Detection_SusiReturnCodeOk__IsThreatIsTrue)
{
    const std::string path = "/tmp/eicar.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": [
        {
            "base64path": "L3BhdGgvZnJvbS9iYXNlNjQ=",
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

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_S_OK)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/path/from/base64", "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestUnitScanner, Detection_SusiReturnCodeError__IsThreatIsTrue_ExtraErrorAdded)
{
    const std::string path = "/tmp/eicar.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": [
        {
            "base64path": "L3BhdGgvZnJvbS9iYXNlNjQ=",
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

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_E_OUTOFMEMORY)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/path/from/base64", "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_EQ(result.errors.at(0).message, "Failed to scan /tmp/eicar.txt due to a susi out of memory error");
}

TEST_F(TestUnitScanner, DetectionsWithErrorAndParseError_SusiReturnCodeError__MultipleErrors)
{
    const std::string path = "/tmp/eicar.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": [
        {
            "base64path": "L3BhdGgvZnJvbS9iYXNlNjQ=",
            "sha256": "sha256",
            "detections": [
                {
                    "threatName": "name",
                    "threatType": "type"
                }
            ],
            "error": "encrypted"
        },
        {}
    ]
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(&susiResult), Return(SUSI_E_OUTOFMEMORY)));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 1);
    EXPECT_EQ(result.detections.at(0), (Detection{ "/path/from/base64", "name", "type", "sha256" }));
    ASSERT_EQ(result.errors.size(), 3);
    EXPECT_EQ(result.errors.at(0).message, "Failed to scan /path/from/base64 as it is password protected");
    EXPECT_THAT(result.errors.at(1).message, StartsWith("Failed to parse SUSI response"));
    EXPECT_EQ(result.errors.at(2).message, "Failed to scan /tmp/eicar.txt due to a susi out of memory error");
}

TEST_F(TestUnitScanner, NullResult_SusiReturnCodeOk__NoError)
{
    const std::string path = "/tmp/clean_file.txt";

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(DoAll(SetArgPointee<3>(nullptr), Return(SUSI_S_OK)));
    EXPECT_CALL(*susiWrapper, freeResult(nullptr));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestUnitScanner, NoDetection_SusiReturnCodeOk_SusiLogsWarning__NoExtraErrorBecauseOfSusiLog)
{
    const std::string path = "/tmp/clean_file.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": []
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(Invoke(
            [&susiResult](
                const char* /* metaData */,
                const char* /* filename */,
                datatypes::AutoFd& /* fd */,
                SusiScanResult** scanResult)
            {
                *scanResult = &susiResult;
                HighestLevelRecorder::record(SUSI_LOG_LEVEL_WARNING);
                return SUSI_S_OK;
            }));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 0);
}

TEST_F(TestUnitScanner, NoDetection_SusiReturnCodeOk_SusiLogsError_UnitScannerLogsErrorButDoesNotReturnError)
{
    const std::string path = "/tmp/clean_file.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": []
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(Invoke(
            [&susiResult](
                const char* /* metaData */,
                const char* /* filename */,
                datatypes::AutoFd& /* fd */,
                SusiScanResult** scanResult)
            {
                *scanResult = &susiResult;
                threat_scanner::susiLogCallback(nullptr, SUSI_LOG_LEVEL_ERROR, "Error message");
                return SUSI_S_OK;
            }));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    EXPECT_EQ(result.detections.size(), 0);
    EXPECT_EQ(result.errors.size(), 0);
    EXPECT_TRUE(appenderContains("ERROR - Error message"));
    EXPECT_TRUE(appenderContains("ERROR - Error logged from SUSI while scanning /tmp/clean_file.txt"));
}

TEST_F(TestUnitScanner, NoDetection_SusiReturnCodeError_SusiLoggedError__NoExtraErrorBecauseOfSusiLog)
{
    const std::string path = "/tmp/clean_file.txt";

    SusiScanResult susiResult{};
    susiResult.scanResultJson = const_cast<char*>(R"({
    "results": []
})");

    auto susiWrapper = std::make_shared<MockSusiWrapper>();
    EXPECT_CALL(*susiWrapper, scanFile(_, path.c_str(), _, _))
        .WillOnce(Invoke(
            [&susiResult](
                const char* /* metaData */,
                const char* /* filename */,
                datatypes::AutoFd& /* fd */,
                SusiScanResult** scanResult)
            {
                *scanResult = &susiResult;
                HighestLevelRecorder::record(SUSI_LOG_LEVEL_ERROR);
                return SUSI_E_OUTOFMEMORY;
            }));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_EQ(result.errors.at(0).message, "Failed to scan /tmp/clean_file.txt due to a susi out of memory error");
}

namespace
{
    class TestUnitScannerMetadataRescan : public MemoryAppenderUsingTests
    {
    protected:
        TestUnitScannerMetadataRescan() : MemoryAppenderUsingTests("ThreatScanner"), usingMemoryAppender_{ *this }
        {
        }

        UsingMemoryAppender usingMemoryAppender_;
        std::shared_ptr<MockSusiWrapper> susiWrapper_ = std::make_shared<MockSusiWrapper>();
        const std::string threatType_ = "threatType";
        const std::string threatName_ = "threatName";
        const std::string path_ = "path";
        const std::string sha256_ = "sha256";
    };
} // namespace

TEST_F(TestUnitScannerMetadataRescan, MetadataRescanPassesCorrectMetadataToSusi)
{
    SusiScanResult susiResult{};

    std::string metadata;

    EXPECT_CALL(*susiWrapper_, metadataRescan)
        .WillOnce(Invoke(
            [&metadata, &susiResult](const char* metaData, SusiScanResult** scanResult)
            {
                metadata = std::string{ metaData };
                *scanResult = &susiResult;
                return SUSI_S_OK;
            }));

    UnitScanner unitScanner{ susiWrapper_ };
    unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);

    const auto parsedMetadata = nlohmann::json::parse(metadata);
    ASSERT_TRUE(parsedMetadata.contains("rescan"));
    ASSERT_TRUE(parsedMetadata["rescan"].contains("sha256"));
    ASSERT_TRUE(parsedMetadata["rescan"].contains("threatName"));
    ASSERT_TRUE(parsedMetadata["rescan"].contains("threatType"));
    ASSERT_TRUE(parsedMetadata["rescan"].contains("path"));
    EXPECT_EQ(parsedMetadata["rescan"]["sha256"], sha256_);
    EXPECT_EQ(parsedMetadata["rescan"]["threatName"], threatName_);
    EXPECT_EQ(parsedMetadata["rescan"]["threatType"], threatType_);
    EXPECT_EQ(parsedMetadata["rescan"]["path"], path_);
}

TEST_F(TestUnitScannerMetadataRescan, MetadataRescanFreesTheResult)
{
    SusiScanResult susiResult{};

    {
        InSequence seq;
        EXPECT_CALL(*susiWrapper_, metadataRescan)
            .WillOnce(Invoke(
                [&susiResult](const char*, SusiScanResult** scanResult)
                {
                    *scanResult = &susiResult;
                    return SUSI_S_OK;
                }));
        EXPECT_CALL(*susiWrapper_, freeResult(&susiResult));
    }

    UnitScanner unitScanner{ susiWrapper_ };
    unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);
}

TEST_F(TestUnitScannerMetadataRescan, MetadataRescanReturnsCorrectResponse)
{
    EXPECT_CALL(*susiWrapper_, metadataRescan)
        .WillOnce(Return(SUSI_S_OK))
        .WillOnce(Return(SUSI_I_THREATPRESENT))
        .WillOnce(Return(SUSI_I_NEEDSFULLSCAN))
        .WillOnce(Return(SUSI_I_UPDATED))
        .WillOnce(Return(SUSI_I_CLEAN));

    UnitScanner unitScanner{ susiWrapper_ };
    const auto result1 = unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);
    const auto result2 = unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);
    const auto result3 = unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);
    const auto result4 = unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);
    const auto result5 = unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);

    EXPECT_EQ(result1, scan_messages::MetadataRescanResponse::undetected);
    EXPECT_EQ(result2, scan_messages::MetadataRescanResponse::threatPresent);
    EXPECT_EQ(result3, scan_messages::MetadataRescanResponse::needsFullScan);
    EXPECT_EQ(result4, scan_messages::MetadataRescanResponse::failed);
    EXPECT_EQ(result5, scan_messages::MetadataRescanResponse::clean);
}

TEST_F(TestUnitScannerMetadataRescan, MetadataRescanReturnsFailureOnUnknownReturnCode)
{
    EXPECT_CALL(*susiWrapper_, metadataRescan).WillOnce(Return(SUSI_I_UPDATED));

    UnitScanner unitScanner{ susiWrapper_ };
    const auto result = unitScanner.metadataRescan(threatType_, threatName_, path_, sha256_);
    EXPECT_EQ(result, scan_messages::MetadataRescanResponse::failed);
    EXPECT_TRUE(appenderContains("Unknown metadata rescan result: "));
}
