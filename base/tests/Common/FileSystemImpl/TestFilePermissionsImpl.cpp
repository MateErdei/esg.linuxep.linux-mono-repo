// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>

#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>

#include <grp.h>
#include <pwd.h>

using namespace Common::FileSystem;
namespace
{
#ifndef ARTISANBUILD
    TEST(FilePermissionsImpl, checkFileCreatedIsNotExecutable)
    {
        Tests::TempDir tempdir;
        tempdir.makeTempDir();
        std::string a = "blah.txt";
        std::string dirpath = tempdir.dirPath() + a;
        tempdir.createFile(dirpath, "");
        auto fileSystem = Common::FileSystem::fileSystem();

        ASSERT_FALSE(fileSystem->isExecutable(dirpath));
    }

    // cppcheck-suppress syntaxError
    TEST(FilePermissionsImpl, checkSophosChmodWorks)
    {
        Tests::TempDir tempdir;
        tempdir.makeTempDir();
        std::string a = "blah.txt";
        std::string dirpath = tempdir.dirPath() + a;
        tempdir.createFile(dirpath, "");
        auto fileSystem = Common::FileSystem::fileSystem();

        fileSystem->makeExecutable(dirpath);
        ASSERT_TRUE(fileSystem->isExecutable(dirpath));
    }
#endif

    // group id tests
    TEST(FilePermissionsImpl, checkGetGroupIdReturnsMinusOneWhenBadGroup)
    {
        EXPECT_THROW(
            Common::FileSystem::filePermissions()->getGroupId("badgroup"), Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetGroupIdReturnsAGroupWhenGoodGroup)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getGroupId("root"), -1);
    }

    TEST(FilePermissionsImpl, checkGetGroupIdReturnstheSameAsGetgrnam)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupId("root"), getgrnam("root")->gr_gid);
    }

    TEST(FilePermissionsImpl, checkGetGroupIdOfRootReturnsZero)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupId("root"), 0);
    }

    // group name tests
    TEST(FilePermissionsImpl, checkGetGroupNameThrowsWhenBadGroup)
    {
        EXPECT_THROW(Common::FileSystem::filePermissions()->getGroupName(-1), Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetGroupNameReturnsAGroupWhenGoodGroup)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getGroupName(0), "");
    }

    TEST(FilePermissionsImpl, checkGetGroupNameReturnstheSameAsGetgrgid)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupName(0), getgrgid(0)->gr_name);
    }

    TEST(FilePermissionsImpl, checkGetGroupNameCalledWith0ReturnsRoot)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupName(0), "root");
    }

    // user id tests
    TEST(FilePermissionsImpl, checkGetUserIdReturnsMinusOneWhenBadUser)
    {
        EXPECT_THROW(
            Common::FileSystem::filePermissions()->getUserId("baduser"), Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetUserIdReturnsAUserWhenGoodUser)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getUserId("root"), -1);
    }

    TEST(FilePermissionsImpl, checkGetUserIdReturnstheSameAsGetpwnam)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserId("root"), getpwnam("root")->pw_uid);
    }

    TEST(FilePermissionsImpl, checkGetUserIdOfRootReturnsZero)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserId("root"), 0);
    }

    TEST(FilePermissionsImpl, checkGetUserAndGroupIdOfRootReturnsZero)
    {
        std::pair<uid_t, gid_t> userAndGroup = std::make_pair(0, 0);
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserAndGroupId("root"), userAndGroup);
    }

    TEST(FilePermissionsImpl, checkGetUserAndGroupIdOfRootDoesNotReturnMinusOne)
    {
        std::pair<uid_t, gid_t> userAndGroup = Common::FileSystem::filePermissions()->getUserAndGroupId("root");
        EXPECT_NE(userAndGroup.first, -1);
        EXPECT_NE(userAndGroup.second, -1);
    }

    TEST(FilePermissionsImpl, checkGetUserAndGruopIdThrowsWhenBadUser)
    {
        EXPECT_THROW(
            Common::FileSystem::filePermissions()->getUserAndGroupId("baduser"),
            Common::FileSystem::IFileSystemException);
    }

    // user name tests
    TEST(FilePermissionsImpl, checkGetUserNameReturnsEmptyStringWhenBadUser)
    {
        EXPECT_THROW(Common::FileSystem::filePermissions()->getUserName(-1), Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetUserNameReturnsAUserWhenGoodUser)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getUserName(0), "");
    }

    TEST(FilePermissionsImpl, checkGetUserNameReturnstheSameAsGetpwid)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserName(0), getpwuid(0)->pw_name);
    }

    TEST(FilePermissionsImpl, checkGetUserNameCalledWith0ReturnsRoot)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserName(0), "root");
    }

    TEST(FilePermissionsImpl, getFilePermissions)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        Tests::TempDir tempdir("", "FilePermissionsImpl_getFilePermissions");
        Path A = tempdir.absPath("A");
        tempdir.createFile("A", "FOOBAR");

        mode_t initialPermissions = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP;
        filePermissions->chmod(A, initialPermissions);
        mode_t initialPermissionsRead = filePermissions->getFilePermissions(A);
        EXPECT_EQ(initialPermissions, initialPermissionsRead);

        mode_t newFilePermissions = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IXUSR;
        filePermissions->chmod(A, newFilePermissions);
        auto newPermissionsRead = filePermissions->getFilePermissions(A);
        EXPECT_NE(initialPermissionsRead, newPermissionsRead);
        EXPECT_EQ(newFilePermissions, newPermissionsRead);
    }

    TEST(FilePermissionsImpl, getAllGroupNamesAndIds)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        auto systemGroups = filePermissions->getAllGroupNamesAndIds();

        ASSERT_FALSE(systemGroups.empty());
        for (const auto& [systemGroupName, systemGid] : systemGroups)
        {
            EXPECT_EQ(filePermissions->getGroupName(systemGid), systemGroupName);
            EXPECT_EQ(filePermissions->getGroupId(systemGroupName), systemGid);
        }
    }

    TEST(FilePermissionsImpl, getAllUserNamesAndIds)
    {
        auto filePermissions = Common::FileSystem::filePermissions();
        auto systemUsers = filePermissions->getAllUserNamesAndIds();

        ASSERT_FALSE(systemUsers.empty());
        for (const auto& [systemUserName, systemUid] : systemUsers)
        {
            EXPECT_EQ(filePermissions->getUserName(systemUid), systemUserName);
            EXPECT_EQ(filePermissions->getUserId(systemUserName), systemUid);
        }
    }

} // namespace