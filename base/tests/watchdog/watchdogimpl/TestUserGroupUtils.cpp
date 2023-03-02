// Copyright 2023 Sophos Limited. All rights reserved.

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "watchdog/watchdogimpl/UserGroupUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TestUserGroupUtils : public LogOffInitializedTests{};

TEST_F(TestUserGroupUtils, CommentsAreStrippedFromRequestedUserAndGroupIdsFileReturningValidJson)
{
    nlohmann::json expectedJson = {
        {"users", {{"username1", 1},{"username2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    std::vector<std::string> configString{
        "#Comment here",
        "#Another comment here",
        "{",
            R"("users": {)",
                R"("username1": 1,)",
                R"("username2": 2)",
            "},",
            R"("groups": {)",
                R"("group1": 1,)",
                R"("group2": 2)",
            "}",
        "}"
    };

    auto strippedConfigString = watchdog::watchdogimpl::stripCommentsFromRequestedUserGroupIdFile(configString);
    EXPECT_EQ(nlohmann::json::parse(strippedConfigString), expectedJson);
}

TEST_F(TestUserGroupUtils, DifferentStructuresOfCommentsAreStrippedFromRequestedUserAndGroupIdsFileReturningValidJson)
{
    nlohmann::json expectedJson = {
        {"users", {{"username1", 1},{"username2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    std::vector<std::string> configString{
        "#Comment here",
        "#Another comment here",
        " #Comment with spacing here",
        " # Comment with more spacing here",
        "               #Comment with lots more spacing here",
        "## Comment with multiple hashes here",
        "{",
            R"("users": {)",
                R"("username1": 1,)",
                R"("username2": 2)",
            "},",
            R"("groups": {)",
                R"("group1": 1,)",
                R"("group2": 2)",
            "}",
        "}"
    };

    auto strippedConfigString = watchdog::watchdogimpl::stripCommentsFromRequestedUserGroupIdFile(configString);
    EXPECT_EQ(nlohmann::json::parse(strippedConfigString), expectedJson);
}

// remapUserAndGroupIds
TEST_F(TestUserGroupUtils, remapUserAndGroupIdsHandlesDirectory)
{
    auto* filesystemMock = new NaggyMock<MockFileSystem>();
    auto* filePermissionsMock = new NaggyMock<MockFilePermissions>();

    std::vector<Path> paths = {"/a/path", "/a"};
    EXPECT_CALL(*filesystemMock, listAllFilesAndDirsInDirectoryTree("/pathroot")).WillOnce(Return(paths));
    EXPECT_CALL(*filesystemMock, isFile("/pathroot")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isDirectory("/pathroot")).WillOnce(Return(true));

    // The file
    EXPECT_CALL(*filePermissionsMock, getUserId("/a/path")).WillOnce(Return(10));
    EXPECT_CALL(*filePermissionsMock, getGroupId("/a/path")).WillOnce(Return(11));
    EXPECT_CALL(*filePermissionsMock, chown("/a/path", 20, 21));

    // The directory
    EXPECT_CALL(*filePermissionsMock, getUserId("/a")).WillOnce(Return(10));
    EXPECT_CALL(*filePermissionsMock, getGroupId("/a")).WillOnce(Return(11));
    EXPECT_CALL(*filePermissionsMock, chown("/a", 20, 21));

    auto fsMock = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    auto fpMock = std::make_unique<Tests::ScopedReplaceFilePermissions>(std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock));

    watchdog::watchdogimpl::remapUserAndGroupIds("/pathroot",10,11,20,21);
}

