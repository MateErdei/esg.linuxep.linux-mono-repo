/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "FileWalkerMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace
{
    class TestFileWalker : public FileWalkerMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
    };
} // namespace