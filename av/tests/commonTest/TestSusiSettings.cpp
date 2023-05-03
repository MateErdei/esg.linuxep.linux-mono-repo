// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "common/ThreatDetector/SusiSettings.h"
#include "tests/common/LogInitializedTests.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace common;

namespace
{
    class TestSusiSettings : public LogInitializedTests
    {};
}

TEST_F(TestSusiSettings, SusiSettingsDefaultsSxlLookupToEnabled)
{
    ThreatDetector::SusiSettings susiSettings;
    ASSERT_TRUE(susiSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsDefaultsAllowListToEmpty)
{
    ThreatDetector::SusiSettings susiSettings;
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 0);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("something"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/a/path"));
    auto copySha = susiSettings.copyAllowListSha256();
    auto copyPath = susiSettings.copyAllowListPath();
    EXPECT_EQ(copySha.size(), 0);
    EXPECT_EQ(copyPath.size(), 0);
}

TEST_F(TestSusiSettings, SusiSettingsDefaultsMachineLearningToEnabled)
{
    ThreatDetector::SusiSettings susiSettings;
    EXPECT_TRUE(susiSettings.isMachineLearningEnabled());
}

TEST_F(TestSusiSettings, KeepsSetValueOfMachineLearning)
{
    ThreatDetector::SusiSettings susiSettings;
    susiSettings.setMachineLearningEnabled(false);
    EXPECT_FALSE(susiSettings.isMachineLearningEnabled());
    susiSettings.setMachineLearningEnabled(true);
    EXPECT_TRUE(susiSettings.isMachineLearningEnabled());
}

TEST_F(TestSusiSettings, compareIncludesMachineLearning)
{
    ThreatDetector::SusiSettings susiSettings;
    susiSettings.setMachineLearningEnabled(true);

    ThreatDetector::SusiSettings susiSettings2;
    susiSettings2.setMachineLearningEnabled(false);

    EXPECT_NE(susiSettings, susiSettings2);
}

TEST_F(TestSusiSettings, SusiSettingsHandlesLoadingEmptyJsonFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(""));

    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 0);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("something"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/a/path"));
    EXPECT_TRUE(susiSettings.isSxlLookupEnabled());
    EXPECT_TRUE(susiSettings.isMachineLearningEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsHandlesLoadingEmptyButValidJsonFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return("{}"));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 0);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("something"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/a/path"));
    EXPECT_TRUE(susiSettings.isSxlLookupEnabled());
    EXPECT_TRUE(susiSettings.isMachineLearningEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsHandleInvalidJson)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return("this is not valid json"));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    ASSERT_EQ(susiSettings.getAllowListSizePath(), 0);
    ASSERT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("something"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/a/path"));
    ASSERT_TRUE(susiSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsHandlesMissingFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).Times(0);
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 0);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("something"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/a/path"));
    EXPECT_TRUE(susiSettings.isSxlLookupEnabled());
    EXPECT_TRUE(susiSettings.isMachineLearningEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsReadsInAllowListSha256)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 1);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 0);
    EXPECT_TRUE(susiSettings.isAllowListedSha256("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/a/path"));
}

TEST_F(TestSusiSettings, SusiSettingsReadsInAllowListPath)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/nowhere"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("not allow listed"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere"));
}

TEST_F(TestSusiSettings, SusiSettingsHandlesReadingInvalidEntriesInAllowListPath)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/nowhere","blah","/path/to/somewhere""/path/to/somewhere/else"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("not allow listed"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/blah"));
}

TEST_F(TestSusiSettings, SusiSettingsReadsInAllowListPathAndSha256)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/nowhere"],"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 1);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);

    EXPECT_FALSE(susiSettings.isAllowListedSha256("not allow listed"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/a/path"));
    EXPECT_TRUE(susiSettings.isAllowListedSha256("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere"));
}

TEST_F(TestSusiSettings, SusiSettingsReadsInSxlLookupEnabled)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_TRUE(susiSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsReadsInSxlLookupDisabled)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":false,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    ASSERT_FALSE(susiSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsReadsInApprovedPuaList)
{
    std::string jsonWithPuaApprovedList = R"({"enableSxlLookup":true,"puaApprovedList":["PUA1", "PUA2"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithPuaApprovedList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    common::ThreatDetector::PuaApprovedList expectedApprovedPuas = {"PUA1","PUA2"};
    auto actual = susiSettings.copyPuaApprovedList();
    ASSERT_EQ(actual, expectedApprovedPuas);
    ASSERT_FALSE(susiSettings.isPuaApproved("not approved PUA"));
    ASSERT_TRUE(susiSettings.isPuaApproved("PUA1"));
    ASSERT_TRUE(susiSettings.isPuaApproved("PUA2"));
}

TEST_F(TestSusiSettings, saveSettings)
{
    ThreatDetector::SusiSettings susiSettings;
    ThreatDetector::AllowList allowlistSha256;
    ThreatDetector::AllowList allowlistPath;
    allowlistSha256.emplace_back("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673");
    allowlistPath.emplace_back("this/is/the/way");
    susiSettings.setAllowListSha256(std::move(allowlistSha256));
    susiSettings.setAllowListPath(std::move(allowlistPath));
    susiSettings.setMachineLearningEnabled(false);
    susiSettings.setSxlLookupEnabled(false);
    ThreatDetector::PuaApprovedList puaApprovedList;
    puaApprovedList.emplace_back("PsExec");
    susiSettings.setPuaApprovedList(std::move(puaApprovedList));

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();

    std::string contents;
    EXPECT_CALL(*filesystemMock, writeFileAtomically("path", _, _, 0))
        .WillOnce(SaveArg<1>(&contents));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(filesystemMock) };

    susiSettings.saveSettings("path", 0);

    std::string expectedContents = R"({"enableSxlLookup":false,"machineLearning":false,"pathAllowList":["this/is/the/way"],"puaApprovedList":["PsExec"],"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    EXPECT_EQ(contents, expectedContents);
}

TEST_F(TestSusiSettings, roundTripSettings)
{
    ThreatDetector::SusiSettings susiSettings;
    ThreatDetector::AllowList allowlistSha256;
    ThreatDetector::AllowList allowlistPath;
    allowlistSha256.emplace_back("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673");
    allowlistPath.emplace_back("/this/is/the/way");
    susiSettings.setAllowListSha256(std::move(allowlistSha256));
    susiSettings.setAllowListPath(std::move(allowlistPath));
    susiSettings.setMachineLearningEnabled(false);
    susiSettings.setSxlLookupEnabled(false);

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();

    std::string contents;
    EXPECT_CALL(*filesystemMock, writeFileAtomically("path", _, _, 0))
        .WillOnce(SaveArg<1>(&contents));

    auto* borrowedMockFs = filesystemMock.get();

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(filesystemMock) };

    susiSettings.saveSettings("path", 0);

    EXPECT_CALL(*borrowedMockFs, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*borrowedMockFs, readFile("path")).WillOnce(Return(contents));

    ThreatDetector::SusiSettings susiSettingsLoaded("path");
    EXPECT_EQ(susiSettings, susiSettingsLoaded);
    EXPECT_EQ(susiSettings.isMachineLearningEnabled(), susiSettingsLoaded.isMachineLearningEnabled());
    EXPECT_EQ(susiSettings.isSxlLookupEnabled(), susiSettingsLoaded.isSxlLookupEnabled());
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), susiSettingsLoaded.getAllowListSizeSha256());
    EXPECT_EQ(susiSettings.getAllowListSizePath(), susiSettingsLoaded.getAllowListSizePath());
}

TEST_F(TestSusiSettings, saveSettingsDoesNotThrowOnFilesystemError)
{
    ThreatDetector::SusiSettings susiSettings;
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, writeFileAtomically("path", _, _, 0)).WillOnce(Throw(
        Common::FileSystem::IFileSystemException("Error")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(filesystemMock) };

    EXPECT_NO_THROW(susiSettings.saveSettings("path", 0));
}

TEST_F(TestSusiSettings, SusiSettingsSetAllowListPathOverridesPreviousSettings)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/nowhere"]})";
    std::vector<std::string> updatedList {"/path/to/somewhere"};
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/path/to/somewhere"));

    susiSettings.setAllowListPath(std::move(updatedList));
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);
    EXPECT_FALSE(susiSettings.isAllowListedPath("/path/to/nowhere"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/somewhere"));
}

TEST_F(TestSusiSettings, SusiSettingsChangeAllowListPathDoesntChangeAllowListSha)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/nowhere"],"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    std::vector<std::string> updatedList {};
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 1);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);
    EXPECT_TRUE(susiSettings.isAllowListedSha256("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere"));

    susiSettings.setAllowListPath(std::move(updatedList));
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 1);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 0);
    EXPECT_TRUE(susiSettings.isAllowListedSha256("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"));
    EXPECT_FALSE(susiSettings.isAllowListedPath("/path/to/nowhere"));
}

TEST_F(TestSusiSettings, SusiSettingsChangeAllowListShaDoesntChangeAllowListPath)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/nowhere/"],"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    std::vector<std::string> updatedList {};
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 1);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);
    EXPECT_TRUE(susiSettings.isAllowListedSha256("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere/file.txt"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere/but/somewhere/file.txt"));

    susiSettings.setAllowListSha256(std::move(updatedList));
    EXPECT_EQ(susiSettings.getAllowListSizeSha256(), 0);
    EXPECT_EQ(susiSettings.getAllowListSizePath(), 1);
    EXPECT_FALSE(susiSettings.isAllowListedSha256("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere/file.txt"));
    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere/but/somewhere/file.txt"));
}

TEST_F(TestSusiSettings, SusiSettingsPathAllowedByGlob)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["*/nowhere/"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere/file.txt"));
}

TEST_F(TestSusiSettings, SusiSettingsPathAllowedByGlobFileType)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/*.txt"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/nowhere/something.txt"));
}

TEST_F(TestSusiSettings, SusiSettingsPathAllowedByExactMatch)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/somewhere/file.txt"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_TRUE(susiSettings.isAllowListedPath("/path/to/somewhere/file.txt"));
}

TEST_F(TestSusiSettings, SusiSettingsPathNotAllowedByIncompletePath)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/path/to/somewhere"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_FALSE(susiSettings.isAllowListedPath("/path/to/somewhere/file.txt"));
}

TEST_F(TestSusiSettings, SusiSettingsWorksWithInvalidAllowListItem)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"pathAllowList":["/a35qy9359qihtdiotqe"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");

    EXPECT_FALSE(susiSettings.isAllowListedPath("/path/to/somewhere/file.txt"));
}