// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

#include "MockThreatDetectorResources.h"

#include "common/MemoryAppender.h"

#include <gtest/gtest.h>


using namespace sspl::sophosthreatdetectorimpl;
using namespace testing;


//Todo - current mock structure gets us to creating the ScanningServerSocket: ln~435 of sophosthreatdetector main
//Todo - Write additional tests LINUXDAR-6030

namespace {
    class TestSophosThreatDetectorMain : public MemoryAppenderUsingTests
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
    };

}

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

TEST_F(TestSophosThreatDetectorMain, throwsIfNoCaphandle)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*mockSysCallWrapper, cap_get_proc()).WillOnce(SetErrnoAndReturn(EINVAL, nullptr));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_MockThreatDetectorResources);
        FAIL() << "Threat detector main::dropCapabilities didnt throw";
    }
    catch (std::exception& ex)
    {
        EXPECT_STREQ(ex.what(), "Failed to drop capabilities after entering chroot");
    }
    EXPECT_TRUE(appenderContains("Failed to get capabilities from call to cap_get_proc: 22 (Invalid argument)"));
}


TEST_F(TestSophosThreatDetectorMain, throwsIfCap_ClearFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*mockSysCallWrapper, cap_clear(_)).WillOnce(SetErrnoAndReturn(EINVAL, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_MockThreatDetectorResources);
        FAIL() << "Threat detector main::dropCapabilities didnt throw";
    }
    catch (std::exception& ex)
    {
        EXPECT_STREQ(ex.what(), "Failed to drop capabilities after entering chroot");
    }
    EXPECT_TRUE(appenderContains("Failed to clear effective capabilities: 22 (Invalid argument)"));
}

TEST_F(TestSophosThreatDetectorMain, throwsIfCap_Set_ProcFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*mockSysCallWrapper, cap_set_proc(_)).WillOnce(SetErrnoAndReturn(EPERM, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_MockThreatDetectorResources);
        FAIL() << "Threat detector main::chroot didnt throw";
    }
    catch (std::exception& ex)
    {
        EXPECT_STREQ(ex.what(), "Failed to drop capabilities after entering chroot");
    }
    EXPECT_TRUE(appenderContains("Failed to set the dropped capabilities: 1 (Operation not permitted)"));
}

TEST_F(TestSophosThreatDetectorMain, runsAsRootIfGetUIDReturnsZero)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    //Todo - This exits prematurely, may not be necessary once LINUXDAR-6030 is implemented
    auto mockSigTermMonitor = std::make_shared<NiceMock<MockSignalHandler>>();
    EXPECT_CALL(*m_MockThreatDetectorResources, createSignalHandler(_)).WillOnce(Return(mockSigTermMonitor));

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));

    EXPECT_CALL(*mockSysCallWrapper, getuid()).WillOnce(Return(0));
    EXPECT_CALL(*mockSigTermMonitor, triggered()).WillOnce(Return(true));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_MockThreatDetectorResources);

    EXPECT_TRUE(waitForLog("Running as root - Skip dropping of capabilities"));
}

TEST_F(TestSophosThreatDetectorMain, throwsIfprctlFails)
{
    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*mockSysCallWrapper, prctl(_,_,_,_,_)).WillOnce(SetErrnoAndReturn(EFAULT, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_MockThreatDetectorResources);
        FAIL() << "Threat detector main::chroot didnt throw";
    }
    catch (std::exception& ex)
    {
        EXPECT_STREQ(ex.what(), "Failed to lock capabilities after entering chroot: 14 (Bad address)");
    }
}

TEST_F(TestSophosThreatDetectorMain, throwsIfchdirFails)
{
    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, chdir(_)).WillOnce(SetErrnoAndReturn(EFAULT, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_MockThreatDetectorResources);
        FAIL() << "Threat detector main::chroot didnt throw";
    }
    catch (std::exception& ex)
    {
        EXPECT_STREQ(ex.what(), "Failed to chdir / after entering chroot 14 (Bad address)");
    }
}

TEST_F(TestSophosThreatDetectorMain, sigTermTerminatesProcess)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger threadRunnerLogger = Common::Logging::getInstance("Common");
    threadRunnerLogger.addAppender(m_sharedAppender);

    auto mockSigTermMonitor = std::make_shared<NiceMock<MockSignalHandler>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSignalHandler(_)).WillOnce(Return(mockSigTermMonitor));
    EXPECT_CALL(*mockSigTermMonitor, triggered()).WillOnce(Return(true));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_MockThreatDetectorResources);

    EXPECT_TRUE(waitForLog("Starting updateCompleteNotifier"));
    ASSERT_TRUE(waitForLog("Sophos Threat Detector received SIGTERM - shutting down"));
    ASSERT_TRUE(waitForLog("Stopping updateCompleteNotifier"));
    ASSERT_TRUE(waitForLog("Joining updateCompleteNotifier"));
}