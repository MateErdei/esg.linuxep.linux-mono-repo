/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <tests/Common/Helpers/TempDir.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <grp.h>
#include <pwd.h>

using namespace Common::FileSystem;
namespace
{

#ifndef ARTISANBUILD
    TEST(FilePermissionsImpl,checkFileCreatedIsNotExecutable)
    {
        Tests::TempDir tempdir;
        tempdir.makeTempDir();
        std::string a ="blah.txt";
        std::string dirpath = tempdir.dirPath()+a;
        tempdir.createFile(dirpath,"");
        auto fileSystem = Common::FileSystem::fileSystem();

        ASSERT_FALSE( fileSystem->isExecutable(dirpath));
    }

    TEST(FilePermissionsImpl,checkSophosChmodWorks)
    {
        Tests::TempDir tempdir;
        tempdir.makeTempDir();
        std::string a ="blah.txt";
        std::string dirpath = tempdir.dirPath()+a;
        tempdir.createFile(dirpath,"");
        auto fileSystem = Common::FileSystem::fileSystem();

        fileSystem->makeExecutable(dirpath);
        ASSERT_TRUE( fileSystem->isExecutable(dirpath));
    }
#endif

    //group id tests
    TEST(FilePermissionsImpl,checkGetGroupIdReturnsMinusOneWhenBadGroup)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupId("badgroup"),-1);
    }

    TEST(FilePermissionsImpl,checkGetGroupIdReturnsAGroupWhenGoodGroup)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getGroupId("root"),-1);
    }

    TEST(FilePermissionsImpl,checkGetGroupIdReturnstheSameAsGetgrnam)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupId("root"),getgrnam("root")->gr_gid);
    }

    TEST(FilePermissionsImpl,checkGetGroupIdOfRootReturnsZero)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupId("root"),0);
    }

    //group name tests
    TEST(FilePermissionsImpl,checkGetGroupNameReturnsEmptyStringWhenBadGroup)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupName(-1),"");
    }

    TEST(FilePermissionsImpl,checkGetGroupNameReturnsAGroupWhenGoodGroup)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getGroupName(0),"");
    }

    TEST(FilePermissionsImpl,checkGetGroupNameReturnstheSameAsGetgrgid)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupName(0),getgrgid(0)->gr_name);
    }

    TEST(FilePermissionsImpl,checkGetGroupNameCalledWith0ReturnsRoot)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getGroupName(0),"root");
    }

    //user id tests
    TEST(FilePermissionsImpl,checkGetUserIdReturnsMinusOneWhenBadUser)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserId("baduser"),-1);
    }

    TEST(FilePermissionsImpl,checkGetUserIdReturnsAUserWhenGoodUser)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getUserId("root"),-1);
    }

    TEST(FilePermissionsImpl,checkGetUserIdReturnstheSameAsGetpwnam)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserId("root"),getpwnam("root")->pw_uid);
    }

    TEST(FilePermissionsImpl,checkGetUserIdOfRootReturnsZero)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserId("root"),0);
    }


    //user name tests
    TEST(FilePermissionsImpl,checkGetUserNameReturnsEmptyStringWhenBadUser)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserName(-1),"");
    }

    TEST(FilePermissionsImpl,checkGetUserNameReturnsAUserWhenGoodUser)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getUserName(0),"");
    }

    TEST(FilePermissionsImpl,checkGetUserNameReturnstheSameAsGetpwid)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserName(0),getpwuid(0)->pw_name);
    }

    TEST(FilePermissionsImpl,checkGetUserNameCalledWith0ReturnsRoot)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getUserName(0),"root");
    }


}