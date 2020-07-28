/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <datatypes/Print.h>
#include <filewalker/FileWalker.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <iostream>
#include <string>
#include <fstream>

using namespace testing;
namespace fs = sophos_filesystem;

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
};

#define BASE "/tmp/TestFileWalkerBackTrackProtection"

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
TEST(TestFileWalkerBacktTrackProtection, backtrackProtection)
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;

    // Create test tree
    fs::create_directories(BASE "/a/b");
    std::ofstream(BASE "/a/b/c");

    fs::create_symlink("b", BASE "/a/d");
    fs::create_symlink(BASE "/a/b", BASE "/a/b/e");
    fs::create_symlink("c", BASE "/a/b/f");

    CallbackImpl callbacks;
    filewalker::FileWalker w(callbacks);
    w.followSymlinks();
    ASSERT_NO_THROW(w.walk(BASE));

    ASSERT_EQ(callbacks.m_paths.size(), 3);
    EXPECT_THAT(callbacks.m_paths, Contains("/tmp/TestFileWalkerBackTrackProtection/a/d/c"));
    EXPECT_THAT(callbacks.m_paths, Contains("/tmp/TestFileWalkerBackTrackProtection/a/b/c"));
    EXPECT_THAT(callbacks.m_paths, AnyOf(Contains("/tmp/TestFileWalkerBackTrackProtection/a/d/e/c")
                                                     , Contains("/tmp/TestFileWalkerBackTrackProtection/a/b/e/c")));

    fs::remove_all(BASE);
}
