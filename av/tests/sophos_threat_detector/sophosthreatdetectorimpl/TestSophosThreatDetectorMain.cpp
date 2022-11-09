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
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
                appConfig.setData("PLUGIN_INSTALL", "/tmp/TestSophosThreatDetectorMain");

                const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
                m_testDir = fs::temp_directory_path();
                m_testDir /= test_info->test_case_name();
                m_testDir /= test_info->name();
                fs::remove_all(m_testDir);
                fs::create_directories(m_testDir);
                fs::current_path(m_testDir);

                const fs::path testUpdateSocketPath = m_testDir / "update_socket";
                m_MockThreatDetectorResources = std::make_shared<NiceMock<MockThreatDetectorResources>>
                    (testUpdateSocketPath);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
        std::shared_ptr<NiceMock<MockThreatDetectorResources>> m_MockThreatDetectorResources;
    };

}

//Todo - current mock structure gets us to creating the ScanningServerSocket ~435
TEST_F(TestSophosThreatDetectorMain, exitsIfChrootFails)
{
    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_MockThreatDetectorResources);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();
    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, chroot("test")).WillOnce(Return(-1));
}


