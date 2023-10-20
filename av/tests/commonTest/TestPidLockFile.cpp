// Copyright 2022-2023 Sophos Limited. All rights reserved.

// Product code
#include "common/PidLockFile.h"
// Test support
#include "tests/common/TestSpecificDirectory.h"
#include "Common/SystemCallWrapper/SystemCallWrapper.h"
// 3rd party
#include <gtest/gtest.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestPidLockFile : public TestSpecificDirectory
    {
    public:
        TestPidLockFile()
            : TestSpecificDirectory("Common")
        {}

        void SetUp() override
        {
            m_testDir = createTestSpecificDirectory();
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            removeTestSpecificDirectory(m_testDir);
        }
        fs::path m_testDir;
    };
}

TEST_F(TestPidLockFile, construction)
{
    auto lockfile = m_testDir / "lockfile";

    EXPECT_FALSE(fs::exists(lockfile));
    {
        common::PidLockFile p{ lockfile };
        EXPECT_TRUE(fs::is_regular_file(lockfile));
    }
    EXPECT_FALSE(fs::exists(lockfile));
}

TEST_F(TestPidLockFile, isLocked)
{
    auto lockfile = m_testDir / "lockfile";
    auto sysCallWrapper = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();

    EXPECT_FALSE(common::PidLockFile::isPidFileLocked(lockfile, sysCallWrapper));
    {
        common::PidLockFile p{ lockfile };
        EXPECT_TRUE(common::PidLockFile::isPidFileLocked(lockfile, sysCallWrapper));
    }
    EXPECT_FALSE(common::PidLockFile::isPidFileLocked(lockfile, sysCallWrapper));
}

TEST_F(TestPidLockFile, readPidNotLocked)
{
    auto lockfile = m_testDir / "lockfile";
    auto sysCallWrapper = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
    EXPECT_EQ(common::PidLockFile::getPidIfLocked(lockfile, sysCallWrapper), 0);
}

TEST_F(TestPidLockFile, readPidLocked)
{
    auto lockfile = m_testDir / "lockfile";
    auto sysCallWrapper = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
    common::PidLockFile p{ lockfile };
    EXPECT_EQ(common::PidLockFile::getPidIfLocked(lockfile, sysCallWrapper), ::getpid());
}
