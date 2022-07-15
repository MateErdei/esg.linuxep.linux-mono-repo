/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "datatypes/sophos_filesystem.h"
#include "modules/manager/scanprocessmonitor/ScanProcessMonitor.h"
#include "ScanProcessMonitorMemoryAppenderUsingTests.h"
#include "tests/datatypes/MockSysCalls.h"

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Threads/AbstractThread.h>

#include <chrono>
#include <fstream>

using namespace plugin::manager::scanprocessmonitor;
namespace fs = sophos_filesystem;

namespace
{
    class TestScanProcessMonitor : public ScanProcessMonitorMemoryAppenderUsingTests
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
            m_systemCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();

        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
        datatypes::ISystemCallWrapperSharedPtr m_systemCallWrapper;
    };
}

TEST_F(TestScanProcessMonitor, constructionWithExistingPath)
{
    EXPECT_NO_THROW(auto m = std::make_unique<ScanProcessMonitor>("", m_systemCallWrapper));
}
