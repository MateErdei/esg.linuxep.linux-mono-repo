// Copyright 2023 Sophos Limited. All rights reserved.

#include <gtest/gtest.h>

#include "Common/FileSystem/IFileSystem.h"
#include "Common/ZipUtilities/ZipUtils.h"

#include "tests/Common/Helpers/LogInitializedTests.h"

namespace
{
    class TestZipUtils : public LogInitializedTests
    {};

    TEST_F(TestZipUtils, unzip_nonExistentFile)
    {
        EXPECT_NE(Common::ZipUtilities::ZipUtils::unzip("nonExistent.zip", "/tmp"), 0);
    }

    TEST_F(TestZipUtils, zip_targetIsDirectory)
    {
        EXPECT_NE(Common::ZipUtilities::ZipUtils::zip("/tmp", "/tmp"), 0);
    }

    TEST_F(TestZipUtils, zip_emptyDirectory)
    {
        std::string testDir = "testDir";
        auto fs = Common::FileSystem::fileSystem();
        fs->makedirs(testDir);
        EXPECT_NE(Common::ZipUtilities::ZipUtils::unzip(testDir, "/tmp"), 0);
        fs->removeFileOrDirectory(testDir);
    }
}