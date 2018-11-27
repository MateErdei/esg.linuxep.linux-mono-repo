/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <Common/FileSystem/IFileSystem.h>

#include "gtest/gtest.h"

using namespace Common::FileSystem;

TEST(TestFileSystemExample, binBashShouldExist) // NOLINT
{
    auto * ifileSystem = Common::FileSystem::fileSystem();
    EXPECT_TRUE(ifileSystem->isExecutable("/bin/bash"));
}
