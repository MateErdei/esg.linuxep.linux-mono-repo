/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <datatypes/Print.h>
#include <filewalker/FileWalker.h>
#include <tests/common/LogInitializedTests.h>

#include <iostream>
#include <string>
#include <fstream>

using namespace testing;
namespace fs = sophos_filesystem;

namespace
{
    class CallbackImpl : public filewalker::IFileWalkCallbacks
    {
    public:
        CallbackImpl() = default;
        std::vector<std::string> m_paths;

        void processFile(const sophos_filesystem::path& filepath, bool symlinkTarget) override
        {
            if (symlinkTarget)
            {
                PRINT("Processing target: " << filepath);
            }
            else
            {
                m_paths.emplace_back(filepath);
                PRINT("Processing: " << filepath);
            }
        }

        bool includeDirectory(const sophos_filesystem::path&) override
        {
            return true;
        }

        bool userDefinedExclusionCheck(const sophos_filesystem::path&) override
        {
            return false;
        }
    };

    class TestFileWalkerBackTrackProtection : public LogInitializedTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_basePath = fs::temp_directory_path() / test_info->test_case_name() / test_info->name();
            fs::remove_all(m_basePath);
            fs::create_directories(m_basePath);
        }

        void TearDown() override
        {
            fs::remove_all(m_basePath);
        }

        fs::path m_basePath;
    };
}

/**
*  The following structure is created:
*      /tmp/TestFileWalkerBackTrackProtection/
*      └── a
*           ├── b
*           │   ├── c
*           │   ├── e -> /tmp/TestFileWalkerBackTrackProtection/a/b
*           │   └── f -> c
*           └── d -> b
*
*  Will scan C three times
*       Once from: /tmp/TestFileWalkerBackTrackProtection/a/d/c
*       Once from: /tmp/TestFileWalkerBackTrackProtection/a/d/e/c
*       Once from: /tmp/TestFileWalkerBackTrackProtection/a/b/c
*/
TEST_F(TestFileWalkerBackTrackProtection, backtrackProtection)
{
    // Create test tree
    fs::create_directories(m_basePath / "a/b");
    std::ofstream(m_basePath / "a/b/c");

    fs::create_symlink("b", m_basePath / "a/d");
    fs::create_symlink(m_basePath / "a/b", m_basePath / "a/b/e");
    fs::create_symlink("c", m_basePath / "a/b/f");

    CallbackImpl callbacks;
    filewalker::FileWalker walker(callbacks);
    walker.followSymlinks();
    ASSERT_NO_THROW(walker.walk(m_basePath));

    ASSERT_EQ(callbacks.m_paths.size(), 3);
    EXPECT_THAT(callbacks.m_paths, Contains(m_basePath / "a/d/c"));
    EXPECT_THAT(callbacks.m_paths, Contains(m_basePath / "a/b/c"));
    EXPECT_THAT(callbacks.m_paths, AnyOf(Contains(m_basePath / "a/d/e/c")
                                                     , Contains(m_basePath / "a/b/e/c")));
}
