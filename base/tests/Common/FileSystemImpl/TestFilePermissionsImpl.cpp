/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/TestHelpers/TempDir.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <grp.h>

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
}