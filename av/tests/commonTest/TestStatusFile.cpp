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
            m_path = m_testDir / "statusFile";
        }

        void TearDown() override
        {
            test_common::removeTestSpecificDirectory(m_testDir);
        }

        fs::path m_testDir;
        fs::path m_path;
    };
}

TEST_F(TestStatusFile, construction)
{
    common::StatusFile f(m_path);
}

TEST_F(TestStatusFile, initiallyNotEnabled)
{
    common::StatusFile f(m_path);
    EXPECT_FALSE(common::StatusFile::isEnabled(m_path));
}

TEST_F(TestStatusFile, isEnabledAfterEnabled)
{
    common::StatusFile f(m_path);
    f.enabled();
    EXPECT_TRUE(common::StatusFile::isEnabled(m_path));
}

TEST_F(TestStatusFile, isDisabledAfterDisable)
{
    common::StatusFile f(m_path);
    f.disabled();
    EXPECT_FALSE(common::StatusFile::isEnabled(m_path));
}

TEST_F(TestStatusFile, isDisabledAfterEnableDisable)
{
    common::StatusFile f(m_path);
    f.enabled();
    f.disabled();
    EXPECT_FALSE(common::StatusFile::isEnabled(m_path));
}

TEST_F(TestStatusFile, isDisabledAfterDestruction)
{
    {
        common::StatusFile f(m_path);
        f.enabled();
    }
    EXPECT_FALSE(common::StatusFile::isEnabled(m_path));
}
