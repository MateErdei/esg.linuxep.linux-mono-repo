// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

#include "MockThreatDetectorResources.h"

#include "common/MemoryAppender.h"
#include <gtest/gtest.h>


using namespace sspl::sophosthreatdetectorimpl;
using namespace testing;

namespace {
    class TestSophosThreatDetectorMain : public MemoryAppenderUsingTests//, public SophosThreatDetectorMain
    {
    public:
        TestSophosThreatDetectorMain() :  MemoryAppenderUsingTests("SophosThreatDetectorImpl")
        {}

        void SetUp() override
        {
            m_appConfig.setData("PLUGIN_INSTALL", m_pluginInstall);

            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);

            //m_mockSysCalls = std::make_shared<NiceMock<MockSystemCallWrapper>>();

            const fs::path testUpdateSocketPath = m_testDir / "update_socket";
            m_MockThreatDetectorResources = std::make_shared<NiceMock<MockThreatDetectorResources>>
            (testUpdateSocketPath);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        const std::string m_pluginInstall = "/tmp/TestSophosThreatDetectorMain";
        Common::ApplicationConfiguration::IApplicationConfiguration& m_appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path m_testDir;
        std::shared_ptr<NiceMock<MockThreatDetectorResources>> m_MockThreatDetectorResources;
        std::shared_ptr<NiceMock<MockSystemCallWrapper>> m_mockSysCalls;
    };

}

//Todo - current mock structure gets us to creating the ScanningServerSocket ~435
TEST_F(TestSophosThreatDetectorMain, throwsIfChrootFails)
{
    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, chroot(_)).WillOnce(Return(-1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_MockThreatDetectorResources);
        FAIL() << "Threat detector main::chroot didnt throw";
    }
    catch (std::exception& ex)
    {
        EXPECT_STREQ(ex.what(), "Failed to chroot to /tmp/TestSophosThreatDetectorMain/chroot: 2 (No such file or directory)");
    }
}

