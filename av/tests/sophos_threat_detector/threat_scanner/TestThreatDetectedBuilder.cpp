// Copyright 2022, Sophos Limited. All rights reserved.

#include "common/MemoryAppender.h"
#include "sophos_threat_detector/threat_scanner/ThreatDetectedBuilder.h"

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>

using namespace threat_scanner;
using namespace common::CentralEnums;

namespace
{
    class TestThreatDetectedBuilder : public MemoryAppenderUsingTests
    {
    public:
        TestThreatDetectedBuilder() : MemoryAppenderUsingTests("ThreatScanner") {}
    };
} // namespace

TEST_F(TestThreatDetectedBuilder, getMainDetectionOneDetectionPathMatches_ReturnsExistingDetection)
{
    const Detection detection1{ "/tmp/eicar.txt", "name", "type", "sha256" };
    Detection result = getMainDetection({ detection1 }, "/tmp/eicar.txt", -1);
    EXPECT_EQ(result, detection1);
}

TEST_F(TestThreatDetectedBuilder, getMainDetectionMultipleDetectionsOnePathMatches_ReturnsMatchingDetection)
{
    const Detection detection1{ "path1", "name1", "type1", "sha256_1" };
    const Detection detection2{ "path2", "name2", "type2", "sha256_2" };
    const Detection detection3{ "path3", "name3", "type3", "sha256_3" };
    Detection result = getMainDetection({ detection1, detection2, detection3 }, "path2", -1);
    EXPECT_EQ(result, detection2);
}

TEST_F(TestThreatDetectedBuilder, getMainDetectionOneDetectionButPathIsShorter_ReturnsNewDetectionWithOriginalPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const Detection detection1{ "/tmp/eicar", "name", "type", "sha256" };
    const int fd = -1;
    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, fd)).Times(1).WillOnce(Return("calculatedSha256"));
    Detection result = getMainDetection({ detection1 }, "/tmp/eicar.txt", fd);
    EXPECT_EQ(result, (Detection{ "/tmp/eicar.txt", "name", "type", "calculatedSha256" }));

    EXPECT_TRUE(appenderContains("Calculated the SHA256 of /tmp/eicar.txt: calculatedSha256"));
    EXPECT_FALSE(appenderContains("Failed to calculate the SHA256"));
}

TEST_F(TestThreatDetectedBuilder, getMainDetectionOneDetectionButPathIsLonger_ReturnsNewDetectionWithOriginalPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const Detection detection1{ "/tmp/eicar.txt/foo", "name", "type", "sha256" };
    const int fd = -1;
    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, fd)).Times(1).WillOnce(Return("calculatedSha256"));
    Detection result = getMainDetection({ detection1 }, "/tmp/eicar.txt", fd);
    EXPECT_EQ(result, (Detection{ "/tmp/eicar.txt", "name", "type", "calculatedSha256" }));

    EXPECT_TRUE(appenderContains("Calculated the SHA256 of /tmp/eicar.txt: calculatedSha256"));
    EXPECT_FALSE(appenderContains("Failed to calculate the SHA256"));
}

TEST_F(
    TestThreatDetectedBuilder,
    getMainDetectionOneDetectionButPathDiffersAndSha256Fails_ReturnsNewDetectionWithUnknownSha256)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const Detection detection1{ "/tmp/eicar.txt/foo", "name", "type", "sha256" };
    const int fd = -1;
    auto mockFileSystem = new StrictMock<MockFileSystem>{};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem = std::unique_ptr<IFileSystem>(mockFileSystem);
    EXPECT_CALL(*mockFileSystem, calculateDigest(_, fd)).Times(1).WillOnce(Throw(std::runtime_error("message")));
    Detection result = getMainDetection({ detection1 }, "/tmp/eicar.txt", fd);
    EXPECT_EQ(result, (Detection{ "/tmp/eicar.txt", "name", "type", "unknown" }));

    EXPECT_FALSE(appenderContains("Calculated the SHA256 of /tmp/eicar.txt: "));
    EXPECT_TRUE(appenderContains("Failed to calculate the SHA256 of /tmp/eicar.txt: message"));
}

TEST_F(TestThreatDetectedBuilder, getMainDetectionNoDetections_ReturnsDummyData)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Detection result = getMainDetection({}, "/tmp/eicar.txt", -1);
    EXPECT_EQ(result, (Detection{ "/tmp/eicar.txt", "unknown", "unknown", "unknown" }));

    EXPECT_TRUE(appenderContains("Trying to get the main detection when no detections were provided"));
    EXPECT_FALSE(appenderContains("Calculated the SHA256 of "));
    EXPECT_FALSE(appenderContains("Failed to calculate the SHA256"));
}

TEST_F(TestThreatDetectedBuilder, buildThreatDetectedBuildsCorrectObject)
{
    auto threatDetected = buildThreatDetected(
        { { "/tmp/eicar.txt", "EICAR-AV-Test", "virus", "sha256" } },
        "/tmp/eicar.txt",
        datatypes::AutoFd{},
        "username",
        scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.userID, "username");
    EXPECT_EQ(threatDetected.threatType, ThreatType::virus);
    EXPECT_EQ(threatDetected.threatName, "EICAR-AV-Test");
    EXPECT_EQ(threatDetected.scanType, scan_messages::E_SCAN_TYPE_ON_DEMAND);
    EXPECT_EQ(threatDetected.notificationStatus, scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE);
    EXPECT_EQ(threatDetected.filePath, "/tmp/eicar.txt");
    EXPECT_EQ(threatDetected.actionCode, scan_messages::E_SMT_THREAT_ACTION_NONE);
    EXPECT_EQ(threatDetected.sha256, "sha256");
    EXPECT_EQ(threatDetected.threatId, "1a209c63-54e9-5080-8078-e283df4a0809");
    EXPECT_EQ(threatDetected.isRemote, false);
    EXPECT_EQ(threatDetected.reportSource, ReportSource::vdl);
}