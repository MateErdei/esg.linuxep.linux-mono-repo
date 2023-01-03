// Copyright 2023 Sophos Limited. All rights reserved.

#include "common/StatusFile.h"

#include "tests/common/LogInitializedTests.h"
#include "tests/common/TestSpecificDirectory.h"

#include <gtest/gtest.h>


namespace fs = sophos_filesystem;

namespace
{
    class TestStatusFile : public LogInitializedTests
    {
    protected:
        void SetUp() override
        {
            m_testDir = test_common::createTestSpecificDirectory();
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            test_common::removeTestSpecificDirectory(m_testDir);
        }

        fs::path m_testDir;
    };
}

TEST_F(TestStatusFile, construction)
{
    common::StatusFile f(m_testDir / "statusFile");
}
