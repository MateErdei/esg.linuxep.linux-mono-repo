// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/threat_scanner/ThreatDetectedBuilder.h"
#include "Common/FileSystem/IFileSystemException.h"
// test headers
#include "tests/common/MemoryAppender.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

using namespace threat_scanner;
using namespace common::CentralEnums;

namespace
{
    class TestThreatDetectedBuilder : public MemoryAppenderUsingTests
    {
    protected:
        TestThreatDetectedBuilder() : MemoryAppenderUsingTests("ThreatScanner"), usingMemoryAppender_{ *this }
        {
        }

        std::unique_ptr<MockFileSystem> mockFileSystem_ = std::make_unique<MockFileSystem>();
        UsingMemoryAppender usingMemoryAppender_;
    };
} // namespace

TEST_F(TestThreatDetectedBuilder, BuildsCorrectObject)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    auto threatDetected = buildThreatDetected(
        { { "/tmp/eicar.txt", "EICAR-AV-Test", "virus", "sha256" } },
        "/tmp/eicar.txt",
        datatypes::AutoFd{},
        "username",
        scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.userID, "username");
    EXPECT_EQ(threatDetected.threatType, "virus");
    EXPECT_EQ(threatDetected.threatName, "EICAR-AV-Test");
    EXPECT_EQ(threatDetected.scanType, scan_messages::E_SCAN_TYPE_ON_DEMAND);
    EXPECT_EQ(threatDetected.filePath, "/tmp/eicar.txt");
    EXPECT_EQ(threatDetected.sha256, "sha256");
    EXPECT_EQ(threatDetected.threatSha256, "sha256");
    EXPECT_EQ(threatDetected.threatId, generateThreatId("/tmp/eicar.txt", "sha256"));
    EXPECT_EQ(threatDetected.isRemote, false);
    EXPECT_EQ(threatDetected.reportSource, ReportSource::vdl);
}

TEST_F(TestThreatDetectedBuilder, weFixShaIfSusiDidNotCalculateIt)
{
    EXPECT_CALL(*mockFileSystem_, calculateDigest(_, -1)).WillOnce(Return("calculatedSha256"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    auto threatDetected = buildThreatDetected(
        { { "/tmp/eicar.txt", "EICAR-AV-Test", "virus", "000000000000000000000000000000000000000000000" } },
        "/tmp/eicar.txt",
        datatypes::AutoFd{},
        "username",
        scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.sha256, "calculatedSha256");
    EXPECT_EQ(threatDetected.threatSha256, "calculatedSha256");
    EXPECT_EQ(threatDetected.threatId, generateThreatId("/tmp/eicar.txt", "calculatedSha256"));

}

TEST_F(TestThreatDetectedBuilder, weHandleFailureToCalculateMissingSha)
{
    EXPECT_CALL(*mockFileSystem_, calculateDigest(_, -1)).WillOnce(Throw(std::runtime_error("")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    auto threatDetected = buildThreatDetected(
        { { "/tmp/eicar.txt", "EICAR-AV-Test", "virus", "000000000000000000000000000000000000000000000" } },
        "/tmp/eicar.txt",
        datatypes::AutoFd{},
        "username",
        scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.sha256, "000000000000000000000000000000000000000000000");
    EXPECT_EQ(threatDetected.threatSha256, "000000000000000000000000000000000000000000000");
    EXPECT_EQ(threatDetected.threatId, generateThreatId("/tmp/eicar.txt", "000000000000000000000000000000000000000000000"));

}

TEST_F(TestThreatDetectedBuilder, MultipleDetectionsOnePathMatchesReturnsMatchingDetection)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    const std::vector<Detection> detections{ { "path1", "name1", "type1", "sha256_1" },
                                             { "path2", "name2", "type2", "sha256_2" },
                                             { "path3", "name3", "type3", "sha256_3" } };

    const auto threatDetected =
        buildThreatDetected(detections, "path2", datatypes::AutoFd{}, "userId", scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.threatType, "type2");
    EXPECT_EQ(threatDetected.threatName, "name2");
    EXPECT_EQ(threatDetected.filePath, "path2");
    EXPECT_EQ(threatDetected.sha256, "sha256_2");
    EXPECT_EQ(threatDetected.threatSha256, "sha256_2");
    EXPECT_EQ(threatDetected.threatId, generateThreatId("path2", "sha256_2"));
}

TEST_F(TestThreatDetectedBuilder, PathIsShorterReturnsOriginalPathWithCalculatedSha256)
{
    EXPECT_CALL(*mockFileSystem_, calculateDigest(_, -1)).WillOnce(Return("calculatedSha256"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    const std::vector<Detection> detections{ { "/tmp/eicar", "name", "type", "sha256" } };
    const auto threatDetected = buildThreatDetected(
        detections, "/tmp/eicar.txt", datatypes::AutoFd{}, "userId", scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.filePath, "/tmp/eicar.txt");
    EXPECT_EQ(threatDetected.sha256, "calculatedSha256");
    EXPECT_EQ(threatDetected.threatSha256, "sha256");
    EXPECT_EQ(threatDetected.threatId, generateThreatId("/tmp/eicar.txt", "calculatedSha256"));

    EXPECT_TRUE(appenderContains("Calculated the SHA256 of /tmp/eicar.txt: calculatedSha256"));
    EXPECT_FALSE(appenderContains("Failed to calculate the SHA256"));
}

TEST_F(TestThreatDetectedBuilder, PathIsLongerReturnsOriginalPathWithCalculatedSha256)
{
    EXPECT_CALL(*mockFileSystem_, calculateDigest(_, -1)).WillOnce(Return("calculatedSha256"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    const std::vector<Detection> detections{ { "/tmp/eicar.txt/foo", "name", "type", "sha256" } };
    const auto threatDetected = buildThreatDetected(
        detections, "/tmp/eicar.txt", datatypes::AutoFd{}, "userId", scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.filePath, "/tmp/eicar.txt");
    EXPECT_EQ(threatDetected.sha256, "calculatedSha256");
    EXPECT_EQ(threatDetected.threatSha256, "sha256");
    EXPECT_EQ(threatDetected.threatId, generateThreatId("/tmp/eicar.txt", "calculatedSha256"));

    EXPECT_TRUE(appenderContains("Calculated the SHA256 of /tmp/eicar.txt: calculatedSha256"));
    EXPECT_FALSE(appenderContains("Failed to calculate the SHA256"));
}

TEST_F(TestThreatDetectedBuilder, PathDiffersAndSha256FailsReturnsUnknownSha256)
{
    EXPECT_CALL(*mockFileSystem_, calculateDigest(_, -1)).WillOnce(Throw(std::runtime_error("message")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    const std::vector<Detection> detections{ { "/tmp/eicar.txt/foo", "name", "type", "sha256" } };
    const auto threatDetected = buildThreatDetected(
        detections, "/tmp/eicar.txt", datatypes::AutoFd{}, "userId", scan_messages::E_SCAN_TYPE_ON_DEMAND);

    EXPECT_EQ(threatDetected.filePath, "/tmp/eicar.txt");
    EXPECT_EQ(threatDetected.sha256, "unknown");
    EXPECT_EQ(threatDetected.threatSha256, "sha256");
    EXPECT_EQ(threatDetected.threatId, generateThreatId("/tmp/eicar.txt", "unknown"));

    EXPECT_FALSE(appenderContains("Calculated the SHA256 of /tmp/eicar.txt: "));
    EXPECT_TRUE(appenderContains("Failed to calculate the SHA256 of /tmp/eicar.txt: message"));
}
