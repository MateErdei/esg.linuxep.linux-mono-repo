// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/Common/FileSystem/IFileSystem.h"
#include "modules/Common/ZipUtilities/ZipUtils.h"

#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

namespace
{
    class TestZipUtils : public LogInitializedTests
    {};

    // cppcheck-suppress syntaxError
    TEST_F(TestZipUtils, unzip_nonExistentFile)
    {
        EXPECT_NE(Common::ZipUtilities::zipUtils().unzip("nonExistent.zip", "/tmp"), 0);
    }

    TEST_F(TestZipUtils, zip_targetIsDirectory)
    {
        EXPECT_NE(Common::ZipUtilities::zipUtils().zip("/tmp", "/tmp", false), 0);
    }

    TEST_F(TestZipUtils, zip_emptyDirectory)
    {
        std::string testDir = "testDir";
        auto fs = Common::FileSystem::fileSystem();
        fs->makedirs(testDir);
        EXPECT_NE(Common::ZipUtilities::zipUtils().unzip(testDir, "/tmp"), 0);
        fs->removeFileOrDirectory(testDir);
    }
}