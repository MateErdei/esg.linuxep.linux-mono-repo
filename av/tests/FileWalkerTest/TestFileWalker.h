/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "FileWalkerMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"

#include "../common/TestSpecificDirectory.h"

namespace fs = sophos_filesystem;

namespace
{
    class TestFileWalker : public FileWalkerMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_testDir = test_common::createTestSpecificDirectory();
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            test_common::removeSpecificDirectory(m_testDir);
        }

        fs::path m_testDir;
    };
} // namespace