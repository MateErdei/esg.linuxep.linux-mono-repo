// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include <gtest/gtest.h>

#include "common/ThreatDetector/SusiSettings.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "tests/common/LogInitializedTests.h"

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
    ASSERT_EQ(susiSettings.getAllowListSize(), 0);
    ASSERT_FALSE(susiSettings.isAllowListed("something"));
}

TEST_F(TestSusiSettings, SusiSettingsHandlesMissingFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("settings.json")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("settings.json")).Times(0);
    ThreatDetector::SusiSettings susiSettings("settings.json");
    ASSERT_EQ(susiSettings.getAllowListSize(), 0);
    ASSERT_FALSE(susiSettings.isAllowListed("something"));
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
    ASSERT_TRUE(susiSettings.isSxlLookupEnabled());
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