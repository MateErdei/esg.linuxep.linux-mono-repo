/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystemImpl/FilePermissionsImpl.h"
#include "../TestHelpers/TempDir.h"
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

    TEST(FilePermissionsImpl,checkSophosGetgrnameReturnsNullWhenBadGroup)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getgrnam("badgroup"),127);
    }

    TEST(FilePermissionsImpl,checkSophosGetgrnameReturnsAGroupWhenGoodGroup)
    {
        EXPECT_NE(Common::FileSystem::filePermissions()->getgrnam("root"),127);
    }

    TEST(FilePermissionsImpl,checkSophosGetgrnameReturnstheSameAsGetgrnam)
    {
        EXPECT_EQ(Common::FileSystem::filePermissions()->getgrnam("root"),getgrnam("root")->gr_gid);
    }

}