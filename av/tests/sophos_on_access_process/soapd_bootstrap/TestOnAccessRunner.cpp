// Copyright 2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "sophos_on_access_process/soapd_bootstrap/OnAccessRunner.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// test headers
#include "tests/sophos_on_access_process/SoapMemoryAppenderUsingTests.h"
#include "tests/common/MemoryAppender.h"

using namespace sophos_on_access_process::soapd_bootstrap;

namespace
{
    class TestOnAccessRunner : public SoapMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir / "var"); // for pidfile
            // fs::create_directories(m_testDir / "var" / "ipc" / "plugins"); // for pluginApi ipc

            m_localSettings = fs::path(m_testDir / "var" / "on_access_local_settings.json");

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir);
            appConfig.setData("PLUGIN_INSTALL", m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
        fs::path m_localSettings;
    };
}

TEST_F(TestOnAccessRunner, checkIfOAShouldBeEnabled_flagDisabled_policyDisabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    
    EXPECT_EQ(OnAccessRunner::checkIfOAShouldBeEnabled(false, false), false);
    EXPECT_TRUE(appenderContains("Overriding policy, on-access will be disabled"));
}

TEST_F(TestOnAccessRunner, checkIfOAShouldBeEnabled_flagDisabled_policyEnabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_EQ(OnAccessRunner::checkIfOAShouldBeEnabled(false, true), false);
    EXPECT_TRUE(appenderContains("Overriding policy, on-access will be disabled"));
}

TEST_F(TestOnAccessRunner, checkIfOAShouldBeEnabled_flagEnabled_policyDisabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_EQ(OnAccessRunner::checkIfOAShouldBeEnabled(true, false), false);
    EXPECT_TRUE(appenderContains("No policy override, following policy settings"));
}

TEST_F(TestOnAccessRunner, checkIfOAShouldBeEnabled_flagEnabled_policyEnabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_EQ(OnAccessRunner::checkIfOAShouldBeEnabled(true, true), true);
    EXPECT_TRUE(appenderContains("No policy override, following policy settings"));
}
