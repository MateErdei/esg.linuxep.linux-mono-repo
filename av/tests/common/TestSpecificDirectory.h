/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "MemoryAppender.h"

#include "datatypes/sophos_filesystem.h"

namespace test_common
{
    inline sophos_filesystem::path createTestSpecificDirectory()
    {
        namespace fs = sophos_filesystem;

        const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        auto testDir = fs::temp_directory_path();
        testDir /= test_info->test_case_name();
        testDir /= test_info->name();
        fs::remove_all(testDir);
        fs::create_directories(testDir);
        return testDir;
    }
}

namespace
{
    class TestSpecificDirectory : public MemoryAppenderUsingTests
    {
    public:
        using MemoryAppenderUsingTests::MemoryAppenderUsingTests;

        static sophos_filesystem::path createTestSpecificDirectory()
        {
            return test_common::createTestSpecificDirectory();
        }
    };


    template<const char* loggerInstanceName>
    class TestSpecificDirectoryTemplate : public TestSpecificDirectory
    {
    public:
        TestSpecificDirectoryTemplate()
            : TestSpecificDirectory(loggerInstanceName)
        {}
    };

}
