// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "MemoryAppender.h"

#include <filesystem>

namespace sophos_filesystem = std::filesystem;

namespace test_common
{
    inline auto remove_all(const sophos_filesystem::path& d)
    {
        namespace fs = sophos_filesystem;
        std::error_code ec{};
        auto removed = fs::remove_all(d, ec);
        if (!ec)
        {
            assert(!fs::exists(d));
            return removed;
        }

        // need to chmod all of the contents to allow delete (assume owned by current user)
        // Can't use our file-walker so will assume that std filesystem walker is ok
        fs::permissions(d, fs::perms::owner_all);
        for(const auto& p: fs::recursive_directory_iterator(d))
        {
            fs::permissions(p, fs::perms::owner_all);
        }

        return fs::remove_all(d);
    }

    inline sophos_filesystem::path createTestSpecificDirectory()
    {
        namespace fs = sophos_filesystem;

        const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        auto testDir = fs::temp_directory_path();
        testDir /= test_info->test_case_name();
        testDir /= test_info->name();
        test_common::remove_all(testDir);
        fs::create_directories(testDir);
        return testDir;
    }

    inline auto removeTestSpecificDirectory(const sophos_filesystem::path& testDir)
    {
        namespace fs = sophos_filesystem;

        fs::current_path(fs::temp_directory_path());
        return test_common::remove_all(testDir);
    }
}

namespace
{
    class TestSpecificDirectory : public MemoryAppenderUsingTests
    {
    public:
        using MemoryAppenderUsingTests::MemoryAppenderUsingTests;

        [[maybe_unused]] static sophos_filesystem::path createTestSpecificDirectory()
        {
            return test_common::createTestSpecificDirectory();
        }

        [[maybe_unused]] static auto removeTestSpecificDirectory(const sophos_filesystem::path& testDir)
        {
            return test_common::removeTestSpecificDirectory(testDir);
        }
    };


    template<const char* loggerInstanceName>
    class [[maybe_unused]] TestSpecificDirectoryTemplate : public TestSpecificDirectory
    {
    public:
        TestSpecificDirectoryTemplate()
            : TestSpecificDirectory(loggerInstanceName)
        {}
    };

}
