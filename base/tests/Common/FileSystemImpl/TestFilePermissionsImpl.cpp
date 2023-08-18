// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/FilePermissionsImpl.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockSysCalls.h"
#include "tests/Common/Helpers/TempDir.h"

#include <gtest/gtest.h>
#include <linux/fs.h>

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

    TEST(FilePermissionsImpl, getUserIdOfDirEntry_pathDoesntExist)
    {
        const std::string path = "/testpath/";
        const std::string expectedMsg = "getUserIdOfDirEntry: " + path + " does not exist";
        auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
        auto filePermissions = Common::FileSystem::filePermissions();

        EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(false));
        Tests::replaceFileSystem(std::move(mockFileSystem));

        try
        {
            filePermissions->getUserIdOfDirEntry(path);
            FAIL() << "Expected exception if path doesnt exist in getUserIdOfDirEntry";
        }
        catch (const IFileSystemException& ex)
        {
            EXPECT_STREQ(ex.what(), expectedMsg.c_str());
        }
    }


    TEST(FilePermissionsImpl, getUserIdOfDirEntry_lstatFails)
    {
        const std::string path = "/testpath/";
        const std::string expectedMsg = "getUserIdOfDirEntry: Calling lstat on " + path + " caused this error: No such file or directory" ;
        auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();

        EXPECT_CALL(*mockSysCalls, lstat(_,_)).WillOnce(Return(ENOENT));
        EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(true));
        Tests::replaceFileSystem(std::move(mockFileSystem));

        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        try
        {
            filePermissions.getUserIdOfDirEntry(path);
            FAIL() << "Expected exception if lstat returns error doesnt exist in getUserIdOfDirEntry";
        }
        catch (const IFileSystemException& ex)
        {
            EXPECT_STREQ(ex.what(), expectedMsg.c_str());
        }
    }

    TEST(FilePermissionsImpl, getUserIdOfDirEntry_Returns_Uid)
    {
        const std::string path = "/testpath/";
        const int expectedUid = 101;
        auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();

        EXPECT_CALL(*mockSysCalls, lstat(_,_)).WillOnce(lstatReturnsUid(expectedUid));
        EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(true));
        Tests::replaceFileSystem(std::move(mockFileSystem));

        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        uid_t uid = filePermissions.getUserIdOfDirEntry(path);
        EXPECT_EQ(uid, expectedUid);
    }

    TEST(FilePermissionsImpl, getGroupIdOfDirEntry_pathDoesntExist)
    {
        const std::string path = "/testpath/";
        const std::string expectedMsg = "getGroupIdOfDirEntry: " + path + " does not exist";
        auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
        auto filePermissions = Common::FileSystem::filePermissions();

        EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(false));
        Tests::replaceFileSystem(std::move(mockFileSystem));

        try
        {
            filePermissions->getGroupIdOfDirEntry(path);
            FAIL() << "Expected exception if path doesnt exist in getGroupIdOfDirEntry";
        }
        catch (const IFileSystemException& ex)
        {
            EXPECT_STREQ(ex.what(), expectedMsg.c_str());
        }
    }

    TEST(FilePermissionsImpl, getGroupIdOfDirEntry_lstatFails)
    {
        const std::string path = "/testpath/";
        const std::string expectedMsg = "getGroupIdOfDirEntry: Calling lstat on " + path + " caused this error: No such file or directory" ;
        auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();

        EXPECT_CALL(*mockSysCalls, lstat(_,_)).WillOnce(Return(ENOENT));
        EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(true));
        Tests::replaceFileSystem(std::move(mockFileSystem));

        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        try
        {
            filePermissions.getGroupIdOfDirEntry(path);
            FAIL() << "Expected exception if lstat returns error doesnt exist in getGroupIdOfDirEntry";
        }
        catch (const IFileSystemException& ex)
        {
            EXPECT_STREQ(ex.what(), expectedMsg.c_str());
        }
    }

    TEST(FilePermissionsImpl, getGroupIdOfDirEntry_Returns_Gid)
    {
        const std::string path = "/testpath/";
        const int expectedGid = 1001;
        auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();

        EXPECT_CALL(*mockSysCalls, lstat(_,_)).WillOnce(lstatReturnsGid(expectedGid));
        EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(true));
        Tests::replaceFileSystem(std::move(mockFileSystem));

        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        uid_t gid = filePermissions.getGroupIdOfDirEntry(path);
        EXPECT_EQ(gid, expectedGid);
    }

    TEST(FilePermissionsImpl, test_getInodeFlags_GetsFlagsFromRegularFile)
    {
        const std::string path = "/testpath/file";
        const int fd = 123;
        struct stat fileStat;
        fileStat.st_mode = __S_IFREG;
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        EXPECT_CALL(*mockSysCalls, _open(_, _)).WillOnce(Return(fd));
        EXPECT_CALL(*mockSysCalls, fstat(fd, _)).WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
        EXPECT_CALL(*mockSysCalls, _ioctl(_, _, An<unsigned long*>()))
            .WillOnce(DoAll(SetArgPointee<2>(10191), Return(0)));
        EXPECT_CALL(*mockSysCalls, _close(fd));
        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        auto flags = filePermissions.getInodeFlags(path);
        EXPECT_EQ(flags, 10191);
    }

    TEST(FilePermissionsImpl, test_getInodeFlags_GetsFlagsFromDir)
    {
        const std::string path = "/testpath/dir/";
        const int fd = 123;
        struct stat fileStat;
        fileStat.st_mode = __S_IFDIR;
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        EXPECT_CALL(*mockSysCalls, _open(_, _)).WillOnce(Return(fd));
        EXPECT_CALL(*mockSysCalls, fstat(fd, _)).WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
        EXPECT_CALL(*mockSysCalls, _ioctl(_, _, An<unsigned long*>()))
            .WillOnce(DoAll(SetArgPointee<2>(10191), Return(0)));
        EXPECT_CALL(*mockSysCalls, _close(fd));
        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        auto flags = filePermissions.getInodeFlags(path);
        EXPECT_EQ(flags, 10191);
    }

    TEST(FilePermissionsImpl, test_getInodeFlags_ThrowsWhenPathIsNotRegularFileOrDir)
    {
        const std::string path = "/testpath/file";
        const int fd = 123;
        struct stat fileStat;
        fileStat.st_mode = fileStat.st_mode & ~__S_IFREG & ~__S_IFDIR;
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        EXPECT_CALL(*mockSysCalls, _open(_, _)).WillOnce(Return(fd));
        EXPECT_CALL(*mockSysCalls, fstat(fd, _)).WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
        EXPECT_CALL(*mockSysCalls, _close(fd));
        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        EXPECT_THROW(filePermissions.getInodeFlags(path), IFileSystemException);
    }

    TEST(FilePermissionsImpl, test_getInodeFlags_ThrowsWhenPathCannotBeOpened)
    {
        const std::string path = "/testpath/file";
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        EXPECT_CALL(*mockSysCalls, _open(_, _)).WillOnce(Return(-1));
        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);
        EXPECT_THROW(filePermissions.getInodeFlags(path), IFileSystemException);
    }

    TEST(FilePermissionsImpl, test_getInodeFlags_ThrowsWhenIoctlCallFails)
    {
        const std::string path = "/testpath/file";
        const int fd = 123;
        struct stat fileStat;
        fileStat.st_mode = __S_IFREG;
        auto mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        EXPECT_CALL(*mockSysCalls, _open(_, _)).WillOnce(Return(fd));
        EXPECT_CALL(*mockSysCalls, fstat(fd, _)).WillOnce(DoAll(SetArgPointee<1>(fileStat), Return(0)));
        EXPECT_CALL(*mockSysCalls, _ioctl(_, _, An<unsigned long*>())).WillOnce(Return(-1));
        EXPECT_CALL(*mockSysCalls, _close(fd));
        auto filePermissions = Common::FileSystem::FilePermissionsImpl(mockSysCalls);

        EXPECT_THROW(filePermissions.getInodeFlags(path), IFileSystemException);
    }

    TEST(FilePermissionsImpl, test_getInodeFlags_WorksOnRealFile)
    {
        Tests::restoreFileSystem();
        std::optional<Path> fullPath = Common::FileSystem::fileSystem()->readlink("/proc/self/exe");
        auto filePermissions = Common::FileSystem::FilePermissionsImpl();
        auto flags = filePermissions.getInodeFlags(fullPath.value());
        // Check a couple of flags that should not be set by default.
        EXPECT_FALSE(flags & FS_IMMUTABLE_FL);
        EXPECT_FALSE(flags & FS_COMPR_FL);
    }

} // namespace