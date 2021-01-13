

/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DownloadReportTestBuilder.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <UpdateSchedulerImpl/UpdateSchedulerProcessor.cpp>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/UtilityImpl/MockFormattedTime.h>

using namespace Common::UtilityImpl;
using namespace ::testing;

class TestUpdateSchedulerProcessorHelperMethods : public LogOffInitializedTests{};

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreWrittenToJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(
            Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(),
            "[\"feature1\",\"feature2\"]"));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> features = { "feature1", "feature2" };
    writeInstalledFeaturesJsonFile(features);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, emptyListOfFeaturesAreWrittenToJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), "[]"));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> features = {};
    writeInstalledFeaturesJsonFile(features);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, noThrowExpectedOnFileSystemError) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(
            Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(),
            "[\"feature1\",\"feature2\"]"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    std::vector<std::string> features = { "feature1", "feature2" };
    EXPECT_NO_THROW(writeInstalledFeaturesJsonFile(features));
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
        exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(R"(["featureA","featureB"])"));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {"featureA", "featureB"};
    std::vector<std::string> actualFeatures = readInstalledFeaturesJsonFile();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromJsonFileWithEmptyList) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
                readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return("[]"));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = readInstalledFeaturesJsonFile();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresAreReadFromEmptyJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
                readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = readInstalledFeaturesJsonFile();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, noThrowWhenFeaturesAreReadFromMalformedJsonFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock,
                readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return("this is not json..."));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = readInstalledFeaturesJsonFile();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}

TEST_F(TestUpdateSchedulerProcessorHelperMethods, featuresDefaultToEmptyListWhenFileDoesNotExist) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock,
                exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()))
        .WillOnce(Return(false));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::vector<std::string> expectedFeatures = {};
    std::vector<std::string> actualFeatures = readInstalledFeaturesJsonFile();
    EXPECT_EQ(expectedFeatures, actualFeatures);
}