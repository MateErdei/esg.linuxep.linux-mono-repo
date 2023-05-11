// Copyright 2023 Sophos All rights reserved.
// Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "sophos_on_access_process/fanotifyhandler/ExecutablePathCache.h"

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
        GTEST_SKIP();
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
        std::string get_executable_path_from_pid_uncached(pid_t pid) override
        {
            count_++;
            return ExecutablePathCache::get_executable_path_from_pid_uncached(pid);
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