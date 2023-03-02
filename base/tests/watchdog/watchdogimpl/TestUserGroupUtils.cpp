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

TEST_F(TestUserGroupUtils, Construction) // NOLINT
{
    std::vector<std::string> fileContents{
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

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readLines(_)).WillOnce(Return(fileContents));
    auto mock = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto returnedValue = watchdog::watchdogimpl::readRequestedUserGroupIds();
    std::cout << returnedValue.dump();
}

// remapUserAndGroupIds
TEST_F(TestUserGroupUtils, remapUserAndGroupIdsHandlesDirectory) // NOLINT
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

