// Copyright 2023 Sophos Limited. All rights reserved.

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFileSystemException.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "watchdog/watchdogimpl/UserGroupUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace watchdog::watchdogimpl;
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

    auto strippedConfigString = stripCommentsFromRequestedUserGroupIdFile(configString);
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

    auto strippedConfigString = stripCommentsFromRequestedUserGroupIdFile(configString);
    EXPECT_EQ(nlohmann::json::parse(strippedConfigString), expectedJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsRemovesRoot)
{
    auto* filePermissionsMock = new NaggyMock<MockFilePermissions>();

    nlohmann::json configJson = {
        {"users", {{"username1", 1},{"root", 0}}},
        {"groups", {{"group1", 1},{"root", 0}}}
    };

    nlohmann::json validatedJson = {
        {"users", {{"username1", 1}}},
        {"groups", {{"group1", 1}}}
    };

    std::map<std::string, gid_t> systemGroups = { { "root", 0 }, { "group2", 2 } };
    std::map<std::string, uid_t> systemUsers = { { "root", 0 }, { "user2", 2 } };

    EXPECT_CALL(*filePermissionsMock, getAllGroupNamesAndIds()).WillOnce(Return(systemGroups));
    EXPECT_CALL(*filePermissionsMock, getAllUserNamesAndIds()).WillOnce(Return(systemUsers));

    auto fpMock = std::make_unique<Tests::ScopedReplaceFilePermissions>(std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock));
    EXPECT_EQ(validateUserAndGroupIds(configJson), validatedJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsReturnsEmptyWhenGroupDatabasesCannotBeRead)
{
    auto* filePermissionsMock = new NaggyMock<MockFilePermissions>();

    nlohmann::json configJson = {
        {"users", {{"username1", 1},{"username2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    EXPECT_CALL(*filePermissionsMock, getAllGroupNamesAndIds()).WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed")));
    auto fpMock = std::make_unique<Tests::ScopedReplaceFilePermissions>(std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock));
    ASSERT_TRUE(validateUserAndGroupIds(configJson).empty());
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsReturnsEmptyWhenUserDatabaseCannotBeRead)
{
    auto* filePermissionsMock = new NaggyMock<MockFilePermissions>();

    nlohmann::json configJson = {
        {"users", {{"username1", 1},{"username2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    EXPECT_CALL(*filePermissionsMock, getAllUserNamesAndIds()).WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed")));
    auto fpMock = std::make_unique<Tests::ScopedReplaceFilePermissions>(std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock));
    ASSERT_TRUE(validateUserAndGroupIds(configJson).empty());
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsHandlesOnlyGroups)
{
    auto* filePermissionsMock = new NaggyMock<MockFilePermissions>();

    nlohmann::json configJson = {
        {"groups", {{"group", 0},{"group1", 1},{"group2", 2}}}
    };

    nlohmann::json validatedJson = {
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    std::map<std::string, gid_t> systemGroups = { { "root", 0 } };
    std::map<std::string, uid_t> systemUsers = { { "root", 0 } };

    EXPECT_CALL(*filePermissionsMock, getAllGroupNamesAndIds()).WillOnce(Return(systemGroups));
    EXPECT_CALL(*filePermissionsMock, getAllUserNamesAndIds()).WillOnce(Return(systemUsers));

    auto fpMock = std::make_unique<Tests::ScopedReplaceFilePermissions>(std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock));
    EXPECT_EQ(validateUserAndGroupIds(configJson), validatedJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsHandlesOnlyUsers)
{
    auto* filePermissionsMock = new NaggyMock<MockFilePermissions>();

    nlohmann::json configJson = {
        {"users", {{"user1", 1},{"user2", 2}}}
    };

    nlohmann::json validatedJson = {
        {"users", {{"user1", 1},{"user2", 2}}}
    };

    std::map<std::string, gid_t> systemGroups = { { "root", 0 } };
    std::map<std::string, uid_t> systemUsers = { { "root", 0 } };

    EXPECT_CALL(*filePermissionsMock, getAllGroupNamesAndIds()).WillOnce(Return(systemGroups));
    EXPECT_CALL(*filePermissionsMock, getAllUserNamesAndIds()).WillOnce(Return(systemUsers));

    auto fpMock = std::make_unique<Tests::ScopedReplaceFilePermissions>(std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock));
    EXPECT_EQ(validateUserAndGroupIds(configJson), validatedJson);
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

