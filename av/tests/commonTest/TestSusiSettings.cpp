// Copyright 2020-2022 Sophos Limited. All rights reserved.

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
    EXPECT_EQ(susiSettings.getAllowListSize(), 0);
    EXPECT_FALSE(susiSettings.isAllowListed("something"));
    auto copy = susiSettings.copyAllowList();
    EXPECT_EQ(copy.size(), 0);
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
    EXPECT_EQ(susiSettings.getAllowListSize(), 0);
    EXPECT_FALSE(susiSettings.isAllowListed("something"));
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
    EXPECT_EQ(susiSettings.getAllowListSize(), 0);
    EXPECT_FALSE(susiSettings.isAllowListed("something"));
    EXPECT_TRUE(susiSettings.isSxlLookupEnabled());
    EXPECT_TRUE(susiSettings.isMachineLearningEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsHandlesMissingFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).Times(0);
    ThreatDetector::SusiSettings susiSettings("settings.json");
    EXPECT_EQ(susiSettings.getAllowListSize(), 0);
    EXPECT_FALSE(susiSettings.isAllowListed("something"));
    EXPECT_TRUE(susiSettings.isSxlLookupEnabled());
    EXPECT_TRUE(susiSettings.isMachineLearningEnabled());
}

TEST_F(TestSusiSettings, SusiSettingsReadsInAllowList)
{
    std::string jsonWithAllowList = R"({"enableSxlLookup":true,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return(jsonWithAllowList));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    ASSERT_EQ(susiSettings.getAllowListSize(), 1);
    ASSERT_FALSE(susiSettings.isAllowListed("not allow listed"));
    ASSERT_TRUE(susiSettings.isAllowListed("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"));
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

TEST_F(TestSusiSettings, SusiSettingsHandleInvalidJson)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).WillOnce(Return("this is not valid json"));
    ThreatDetector::SusiSettings susiSettings("settings.json");
    ASSERT_EQ(susiSettings.getAllowListSize(), 0);
    ASSERT_FALSE(susiSettings.isAllowListed("not allow listed"));
    ASSERT_TRUE(susiSettings.isSxlLookupEnabled());
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
    ThreatDetector::AllowList allowlist;
    allowlist.emplace_back("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673");
    susiSettings.setAllowList(std::move(allowlist));
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

    std::string expectedContents = R"({"enableSxlLookup":false,"machineLearning":false,"puaApprovedList":["PsExec"],"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    EXPECT_EQ(contents, expectedContents);
}

TEST_F(TestSusiSettings, roundTripSettings)
{
    ThreatDetector::SusiSettings susiSettings;
    ThreatDetector::AllowList allowlist;
    allowlist.emplace_back("42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673");
    susiSettings.setAllowList(std::move(allowlist));
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
    EXPECT_EQ(susiSettings.getAllowListSize(), susiSettingsLoaded.getAllowListSize());
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
