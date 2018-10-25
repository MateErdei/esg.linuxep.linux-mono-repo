/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Common/FilePermissions/IFilePermissions.h"
#include "Common/FilePermissionsImpl/FilePermissionsImpl.h"
#include "../TestHelpers/TempDir.h"
#include <grp.h>

using namespace Common::FilePermissions;
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
        EXPECT_EQ(Common::FilePermissions::filePermissions()->sophosGetgrnam("badgroup"),nullptr);
    }

    TEST(FilePermissionsImpl,checkSophosGetgrnameReturnsAGroupWhenGoodGroup)
    {
        EXPECT_NE(Common::FilePermissions::filePermissions()->sophosGetgrnam("root"),nullptr);
    }

    TEST(FilePermissionsImpl,checkSophosGetgrnameReturnstheSameAsGetgrnam)
    {
        EXPECT_EQ(Common::FilePermissions::filePermissions()->sophosGetgrnam("root"),getgrnam("root"));
    }

}