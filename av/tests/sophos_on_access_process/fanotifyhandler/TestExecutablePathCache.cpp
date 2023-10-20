// Copyright 2022-2023 Sophos Limited. All rights reserved.

// class under test:
#include "sophos_on_access_process/fanotifyhandler/ExecutablePathCache.h"

// test

#include "FanotifyHandlerMemoryAppenderUsingTests.h"
#include "tests/common/TestSpecificDirectory.h"

#include <gtest/gtest.h>

using namespace sophos_on_access_process::fanotifyhandler;

namespace
{
    class TestExecutablePathCache : public FanotifyHandlerMemoryAppenderUsingTests
    {
    public:
    };

    std::string test_get_executable_path_from_pid(pid_t pid)
    {
        ExecutablePathCache cache;
        return cache.get_executable_path_from_pid(pid);
    }
}

TEST_F(TestExecutablePathCache, get_executable_path_from_pid_init)
{
    if (geteuid() != 0)
    {
        GTEST_SKIP() << "Can't get euid from init if not running as root";
    }
    auto initExe = test_get_executable_path_from_pid(1);
    EXPECT_NE(initExe, "");
}

TEST_F(TestExecutablePathCache, get_executable_path_from_pid_self)
{
    auto exe = test_get_executable_path_from_pid(getpid());
    EXPECT_NE(exe, "");
}

TEST_F(TestExecutablePathCache, get_executable_path_from_pid_empty_from_bad_pid)
{
    auto exe = test_get_executable_path_from_pid(0);
    EXPECT_EQ(exe, "");
}

namespace
{
    class CountingCache : public ExecutablePathCache
    {
    public:
        int count_ = 0;
        std::string get_executable_path_from_pid_uncached(pid_t pid, std::error_code& ec) override
        {
            count_++;
            return ExecutablePathCache::get_executable_path_from_pid_uncached(pid, ec);
        }

        void setCacheLifetime(std::chrono::milliseconds lifetime)
        {
            cache_lifetime_ = lifetime;
        }
    };
}

TEST_F(TestExecutablePathCache, count_first_lookup)
{
    CountingCache cache;
    auto exe = cache.get_executable_path_from_pid(getpid());
    EXPECT_NE(exe, "");
    EXPECT_EQ(cache.count_, 1);
}

TEST_F(TestExecutablePathCache, caches_second_lookup)
{
    CountingCache cache;
    auto exe1 = cache.get_executable_path_from_pid(getpid());
    auto exe2 = cache.get_executable_path_from_pid(getpid());
    EXPECT_NE(exe1, "");
    EXPECT_EQ(exe1, exe2);
    EXPECT_EQ(cache.count_, 1);
}

TEST_F(TestExecutablePathCache, cache_expires)
{
    CountingCache cache;
    // Set the cache lifetime to 1ms so that the test doesn't take too long
    cache.setCacheLifetime(std::chrono::milliseconds(1));
    auto exe1 = cache.get_executable_path_from_pid(getpid());
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    auto exe2 = cache.get_executable_path_from_pid(getpid());
    EXPECT_NE(exe1, "");
    EXPECT_EQ(exe1, exe2);
    EXPECT_EQ(cache.count_, 2);
}

TEST_F(TestExecutablePathCache, caches_after_expire)
{
    CountingCache cache;
    // Set the cache lifetime to 1ms so that the test doesn't take too long
    cache.setCacheLifetime(std::chrono::milliseconds(1));
    auto exe1 = cache.get_executable_path_from_pid(getpid());
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto exe2 = cache.get_executable_path_from_pid(getpid());
    auto exe3 = cache.get_executable_path_from_pid(getpid());
    EXPECT_NE(exe1, "");
    EXPECT_EQ(exe1, exe2);
    EXPECT_EQ(exe1, exe3);
    EXPECT_EQ(cache.count_, 2);
}

namespace
{
    namespace fs = sophos_filesystem;
    class OverrideDirectoryExecutablePathCache : public ExecutablePathCache
    {
    public:
        void setBase(fs::path& path)
        {
            proc_ = path;
        }
    };
}

TEST_F(TestExecutablePathCache, do_not_cache_empty)
{
    namespace fs = sophos_filesystem;
    test_common::TestSpecificDirectoryScope testDirScope;
    auto& testDir = testDirScope.testSpecificDirectory_;
    OverrideDirectoryExecutablePathCache cache;
    cache.setBase(testDir);

    auto ret1 = cache.get_executable_path_from_pid(1);
    EXPECT_TRUE(ret1.empty());

    fs::create_directory(testDir / "1");
    fs::create_symlink("/bin/bash", testDir / "1" / "exe");

    auto ret2 = cache.get_executable_path_from_pid(1);
    EXPECT_EQ(ret2, "/bin/bash");
}