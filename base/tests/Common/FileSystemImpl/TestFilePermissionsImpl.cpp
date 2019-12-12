/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/TempDir.h>

#include <grp.h>
#include <pwd.h>

using namespace Common::FileSystem;
namespace
{
#ifndef ARTISANBUILD
    TEST(FilePermissionsImpl, checkFileCreatedIsNotExecutable) // NOLINT
    {
        Tests::TempDir tempdir;
        tempdir.makeTempDir();
        std::string a = "blah.txt";
        std::string dirpath = tempdir.dirPath() + a;
        tempdir.createFile(dirpath, "");
        auto fileSystem = Common::FileSystem::fileSystem();

        ASSERT_FALSE(fileSystem->isExecutable(dirpath));
    }

    TEST(FilePermissionsImpl, checkSophosChmodWorks) // NOLINT
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
    TEST(FilePermissionsImpl, checkGetGroupIdReturnsMinusOneWhenBadGroup) // NOLINT
    {
        EXPECT_THROW( // NOLINT
            Common::FileSystem::filePermissions()->getGroupId("badgroup"),
            Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetGroupIdReturnsAGroupWhenGoodGroup) // NOLINT
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getGroupId("root"), -1);
    }

    TEST(FilePermissionsImpl, checkGetGroupIdReturnstheSameAsGetgrnam) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupId("root"), getgrnam("root")->gr_gid);
    }

    TEST(FilePermissionsImpl, checkGetGroupIdOfRootReturnsZero) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupId("root"), 0);
    }

    // group name tests
    TEST(FilePermissionsImpl, checkGetGroupNameThrowsWhenBadGroup) // NOLINT
    {
        EXPECT_THROW( // NOLINT
            Common::FileSystem::filePermissions()->getGroupName(-1),
            Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetGroupNameReturnsAGroupWhenGoodGroup) // NOLINT
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getGroupName(0), "");
    }

    TEST(FilePermissionsImpl, checkGetGroupNameReturnstheSameAsGetgrgid) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupName(0), getgrgid(0)->gr_name);
    }

    TEST(FilePermissionsImpl, checkGetGroupNameCalledWith0ReturnsRoot) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupName(0), "root");
    }

    // user id tests
    TEST(FilePermissionsImpl, checkGetUserIdReturnsMinusOneWhenBadUser) // NOLINT
    {
        EXPECT_THROW( // NOLINT
            Common::FileSystem::filePermissions()->getUserId("baduser"),
            Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetUserIdReturnsAUserWhenGoodUser) // NOLINT
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getUserId("root"), -1);
    }

    TEST(FilePermissionsImpl, checkGetUserIdReturnstheSameAsGetpwnam) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserId("root"), getpwnam("root")->pw_uid);
    }

    TEST(FilePermissionsImpl, checkGetUserIdOfRootReturnsZero) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserId("root"), 0);
    }

    // user name tests
    TEST(FilePermissionsImpl, checkGetUserNameReturnsEmptyStringWhenBadUser) // NOLINT
    {
        EXPECT_THROW( // NOLINT
            Common::FileSystem::filePermissions()->getUserName(-1),
            Common::FileSystem::IFileSystemException);
    }

    TEST(FilePermissionsImpl, checkGetUserNameReturnsAUserWhenGoodUser) // NOLINT
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getUserName(0), "");
    }

    TEST(FilePermissionsImpl, checkGetUserNameReturnstheSameAsGetpwid) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserName(0), getpwuid(0)->pw_name);
    }

    TEST(FilePermissionsImpl, checkGetUserNameCalledWith0ReturnsRoot) // NOLINT
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserName(0), "root");
    }

    TEST(FilePermissionsImpl, getFilePermissions) // NOLINT
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

} // namespace