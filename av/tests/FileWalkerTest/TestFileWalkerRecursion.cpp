/******************************************************************************************************

Copyright 2019-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestFileWalker.h"
#include "MockCallbacks.h"

#include <filewalker/FileWalker.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <fstream>
#include <string>

#include <sys/resource.h>

namespace fs = sophos_filesystem;

using namespace ::testing;

namespace
{
    class TestFileWalkerRecursion : public TestFileWalker
    {
    };
}

TEST_F(TestFileWalkerRecursion, hugeFilePathStartFromPathRoot) // NOLINT
{
    const fs::path& startingPath = fs::current_path();
    fs::create_directories("TestHugePathFileWalker");
    fs::current_path("TestHugePathFileWalker");

    int depth;
    try
    {
        for (depth = 0; depth < 100; ++depth)
        {
            fs::create_directories("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
    }
    catch (fs::filesystem_error& e)
    {
        // depending on starting path, we may not be able to create 100 subdirs
        ASSERT_GT(depth, 90);
    }

    fs::current_path(startingPath);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk("TestHugePathFileWalker"));

    auto traverse_and_delete_huge_directory = [](const sophos_filesystem::path& startingPath, int targetDirectory) {
      fs::current_path("TestHugePathFileWalker");
      for (int depth = 0; depth < targetDirectory; ++depth)
      {
          fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
      }
      fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
      fs::current_path(startingPath);
    };

    traverse_and_delete_huge_directory(startingPath, 70);
    traverse_and_delete_huge_directory(startingPath, 35);
    fs::remove_all("TestHugePathFileWalker");
}


TEST_F(TestFileWalkerRecursion, hugeStartingFilePath) // NOLINT
{
    const fs::path& startingPath = fs::current_path();
    fs::create_directories("TestHugePathFileWalker");
    fs::current_path("TestHugePathFileWalker");

    int depth;
    try
    {
        for (depth = 0; depth < 100; ++depth)
        {
            fs::create_directories("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
    }
    catch (fs::filesystem_error& e)
    {
        // depending on starting path, we may not be able to create 100 subdirs
        ASSERT_GT(depth, 90);
    }

    const fs::path& pathToScan = fs::current_path();
    fs::current_path(startingPath);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk(pathToScan);
        FAIL() << "walk() didn't throw";
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to start scan: Starting Path too long: File name too long"));
        EXPECT_EQ(e.code().value(), ENAMETOOLONG);
    }

    auto traverse_and_delete_huge_directory = [](const sophos_filesystem::path& startingPath, int targetDirectory)
    {
      fs::current_path("TestHugePathFileWalker");
      for (int depth = 0; depth < targetDirectory; ++depth)
      {
          fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
      }
      fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
      fs::current_path(startingPath);
    };

    traverse_and_delete_huge_directory(startingPath, 70);
    traverse_and_delete_huge_directory(startingPath, 35);
    fs::remove_all("TestHugePathFileWalker");
}


TEST_F(TestFileWalkerRecursion, deepFilePathStartFromPathRoot) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const fs::path& startingPath = fs::current_path();
    fs::create_directories("deep_path");
    fs::current_path("deep_path");

    for (int depth = 0; depth < 2; ++depth)
    {
        fs::create_directories("0/1/2/3/4/5/6/7/8/9");
        fs::current_path("0/1/2/3/4/5/6/7/8/9");
    }
    std::ofstream("file.txt").close();

    fs::current_path(startingPath);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);

    EXPECT_CALL(*callbacks, processFile(EndsWith("/file.txt"), _)).Times(0);

    // set the rlimit for number of open file descriptors
    struct rlimit old_limit = {};
    ASSERT_EQ(getrlimit(RLIMIT_NOFILE, &old_limit), 0);
    struct rlimit new_limit = { .rlim_cur = 16, .rlim_max = old_limit.rlim_max };
    ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &new_limit), 0);

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk("deep_path"));

    // need to restore the rlimit now, otherwise fs::remove_all() fails
    EXPECT_EQ(setrlimit(RLIMIT_NOFILE, &old_limit), 0);

    fs::remove_all("deep_path");
}

TEST_F(TestFileWalkerRecursion, cannotIterate) // NOLINT
{
    const fs::path& startingPath = fs::current_path();
    fs::create_directories("directory/subdir");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);

    // set the rlimit for number of open file descriptors
    struct rlimit old_limit = {};
    ASSERT_EQ(getrlimit(RLIMIT_NOFILE, &old_limit), 0);
    struct rlimit new_limit = { .rlim_cur = 3, .rlim_max = old_limit.rlim_max };
    ASSERT_EQ(setrlimit(RLIMIT_NOFILE, &new_limit), 0);

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk("directory"));

    // need to restore the rlimit now, otherwise fs::remove_all() fails
    EXPECT_EQ(setrlimit(RLIMIT_NOFILE, &old_limit), 0);

    fs::remove_all("directory");
}
