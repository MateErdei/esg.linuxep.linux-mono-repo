// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#include "../SoapMemoryAppenderUsingTests.h"
#include "common/MemoryAppender.h"
#include "datatypes/MockSysCalls.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "sophos_on_access_process/soapd_bootstrap/SoapdBootstrap.h"

#include <gtest/gtest.h>

using namespace sophos_on_access_process::soapd_bootstrap;

namespace
{
    class TestSoapdBootstrap : public SoapMemoryAppenderUsingTests
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
            // fs::create_directories(m_testDir / "var" / "ipc" / "plugins"); // for pluginapi ipc

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
            appConfig.setData("PLUGIN_INSTALL", m_testDir );

            m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            ON_CALL(*m_mockSysCallWrapper, setrlimit(_, _)).WillByDefault(Return(0));
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper;
    };
}

TEST_F(TestSoapdBootstrap, checkIfOAShouldBeEnabled_flagDisabled_policyDisabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_EQ(SoapdBootstrap::checkIfOAShouldBeEnabled(false, false), false);
    EXPECT_TRUE(appenderContains("Overriding policy, on-access will be disabled"));
}

TEST_F(TestSoapdBootstrap, checkIfOAShouldBeEnabled_flagDisabled_policyEnabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_EQ(SoapdBootstrap::checkIfOAShouldBeEnabled(false, true), false);
    EXPECT_TRUE(appenderContains("Overriding policy, on-access will be disabled"));
}

TEST_F(TestSoapdBootstrap, checkIfOAShouldBeEnabled_flagEnabled_policyDisabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_EQ(SoapdBootstrap::checkIfOAShouldBeEnabled(true, false), false);
    EXPECT_TRUE(appenderContains("No policy override, following policy settings"));
}

TEST_F(TestSoapdBootstrap, checkIfOAShouldBeEnabled_flagEnabled_policyEnabled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_EQ(SoapdBootstrap::checkIfOAShouldBeEnabled(true, true), true);
    EXPECT_TRUE(appenderContains("No policy override, following policy settings"));
}

TEST_F(TestSoapdBootstrap, initialize)
{
    auto SoapdInstance = SoapdBootstrap(m_mockSysCallWrapper);
}

TEST_F(TestSoapdBootstrap, runSoapd)
{
    int ret = SoapdBootstrap::runSoapd(m_mockSysCallWrapper);
    EXPECT_EQ(ret, 1);
}
