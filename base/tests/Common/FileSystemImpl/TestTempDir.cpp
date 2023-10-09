/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "modules/Common/FileSystem/IFileSystem.h"
#include "modules/Common/FileSystemImpl/TempDir.h"
#include <gtest/gtest.h>

// cppcheck-suppress syntaxError
TEST(TestTempDir, ConstructorCreatesTheTemporaryDirectory) // NOLINT
{
    auto fileSystem = Common::FileSystem::fileSystem();
    Common::FileSystemImpl::TempDir td("", "ConstructorCreatesTheTemporaryDirectory");
    ASSERT_TRUE(fileSystem->isDirectory(td.dirPath()));
}

TEST(TestTempDir, DestructorRemovesTheTemporaryDirectory) // NOLINT
{
    auto fileSystem = Common::FileSystem::fileSystem();
    std::string dirPath;
    {
        Common::FileSystemImpl::TempDir td("", "DestructorRemovesTheTemporaryDirectory");
        dirPath = td.dirPath();
        ASSERT_TRUE(fileSystem->isDirectory(dirPath));
    }
    EXPECT_FALSE(fileSystem->isDirectory(dirPath));
}
