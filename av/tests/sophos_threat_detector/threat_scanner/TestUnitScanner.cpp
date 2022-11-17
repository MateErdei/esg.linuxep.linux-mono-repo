// Copyright 2022, Sophos Limited. All rights reserved.

#include "MockSusiWrapper.h"

#include "sophos_threat_detector/threat_scanner/SusiLogger.h"
#include "sophos_threat_detector/threat_scanner/UnitScanner.h"

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace threat_scanner;

namespace
{
    class TestUnitScanner : public LogOffInitializedTests
    {
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

TEST_F(TestUnitScanner, NoDetection_SusiReturnCodeOk_SusiLogsError__AddsErrorBecauseOfSusiLog)
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
                return SUSI_S_OK;
            }));
    EXPECT_CALL(*susiWrapper, freeResult(&susiResult));

    UnitScanner unitScanner{ susiWrapper };
    datatypes::AutoFd fd;
    ScanResult result = unitScanner.scan(fd, path);

    ASSERT_EQ(result.detections.size(), 0);
    ASSERT_EQ(result.errors.size(), 1);
    EXPECT_EQ(result.errors.at(0).message, "Error logged from SUSI while scanning /tmp/clean_file.txt");
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