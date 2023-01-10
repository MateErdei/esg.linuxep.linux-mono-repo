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

            m_MockThreatDetectorResources = std::make_shared<NiceMock<MockThreatDetectorResources>>(m_testDir);
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

    ASSERT_TRUE(waitForLog("Starting updateCompleteNotifier"));
    EXPECT_TRUE(waitForLog("Sophos Threat Detector received SIGTERM - shutting down"));
    EXPECT_TRUE(waitForLog("Stopping updateCompleteNotifier"));
    EXPECT_TRUE(waitForLog("Joining updateCompleteNotifier"));
}


TEST_F(TestSophosThreatDetectorMain, logsWhenAttemptDNSQueryFailsWithEAI_SYSTEM)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, getaddrinfo(_,_,_,_)).WillOnce(SetErrnoAndReturn(EIO, EAI_SYSTEM));
    EXPECT_CALL(*mockSysCallWrapper, ppoll(_,_,_,_)).WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_MockThreatDetectorResources);

    EXPECT_TRUE(waitForLog("Failed DNS query of 4.sophosxl.net: system error in getaddrinfo: Input/output error"));
}

TEST_F(TestSophosThreatDetectorMain, logsWhenAttemptDNSQueryFailsWithOtherError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, getaddrinfo(_,_,_,_)).WillOnce(Return(EAI_AGAIN));
    EXPECT_CALL(*mockSysCallWrapper, ppoll(_,_,_,_)).WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_MockThreatDetectorResources);
    EXPECT_TRUE(waitForLog("Failed DNS query of 4.sophosxl.net: error in getaddrinfo: Temporary failure in name resolution"));
}

TEST_F(TestSophosThreatDetectorMain, FailureToUpdateScannerFactoryExitsInnerMain)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto mockScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSusiScannerFactory(_,_,_)).WillOnce(Return(mockScannerFactory));
    EXPECT_CALL(*mockScannerFactory, update()).WillOnce(Return(false));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_GENERIC_FAILURE, treatDetectorMain.inner_main(m_MockThreatDetectorResources));

    EXPECT_TRUE(waitForLog("[FATAL] Update of scanner at startup failed exiting threat detector main"));
    getLogger().setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
}

TEST_F(TestSophosThreatDetectorMain, continuesWhenReceivesErrorEINTR)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, ppoll(_,_,_,_))
        .WillOnce(SetErrnoAndReturn(EINTR, -1))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_CLEAN_SUCCESS, treatDetectorMain.inner_main(m_MockThreatDetectorResources));

    EXPECT_TRUE(waitForLog("Ignoring EINTR from ppoll"));
}

TEST_F(TestSophosThreatDetectorMain, exitsWhenPpollReturnsOtherError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));
    EXPECT_CALL(*mockSysCallWrapper, ppoll(_,_,_,_)).WillOnce(SetErrnoAndReturn(EACCES, -1));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_GENERIC_FAILURE, treatDetectorMain.inner_main(m_MockThreatDetectorResources));

    EXPECT_TRUE(waitForLog("Failed to poll signals. Error: "));
}

class TestSophosThreatDetectorMainPpollErrorsParameterized
    : public ::testing::TestWithParam<std::pair<int, int>>
{
protected:
    void SetUp() override
    {
        m_appConfig.setData("PLUGIN_INSTALL", m_pluginInstall);

        const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_testDir = fs::temp_directory_path();
        m_testDir /= test_info->test_case_name();
        //m_testDir /= test_info->name();
        fs::remove_all(m_testDir);
        fs::create_directories(m_testDir);
        fs::current_path(m_testDir);

        m_MockThreatDetectorResources = std::make_shared<NiceMock<MockThreatDetectorResources>>(m_testDir);
        m_loggingSetup = Common::Logging::LOGOFFFORTEST();
    }

    void TearDown() override
    {
        fs::current_path(fs::temp_directory_path());
        fs::remove_all(m_testDir);
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    fs::path m_testDir;
    const std::string m_pluginInstall = "/tmp/TestSophosThreatDetectorMainPpollErrorsParameterized";
    Common::ApplicationConfiguration::IApplicationConfiguration& m_appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    std::shared_ptr<NiceMock<MockThreatDetectorResources>> m_MockThreatDetectorResources;
    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
};

INSTANTIATE_TEST_SUITE_P(
    TestSophosThreatDetectorMainErrorHandling,
    TestSophosThreatDetectorMainPpollErrorsParameterized,
    ::testing::Values(
        std::make_pair(EFAULT, common::E_GENERIC_FAILURE),
        std::make_pair(EINTR, common::E_CLEAN_SUCCESS),
        std::make_pair(EINVAL, common::E_GENERIC_FAILURE),
        std::make_pair(ENOMEM, common::E_GENERIC_FAILURE),
        std::make_pair(EAGAIN, common::E_GENERIC_FAILURE)
                 ));

TEST_P(TestSophosThreatDetectorMainPpollErrorsParameterized, innerMainReturnPpollError)
{
    auto mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*mockSysCallWrapper, ppoll(_,_,_,_))
        .WillOnce(SetErrnoAndReturn(std::get<0>(GetParam()), -1))
        //Is will repeatedly because some params wont call this at all
        .WillRepeatedly(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_MockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(mockSysCallWrapper));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(std::get<1>(GetParam()), treatDetectorMain.inner_main(m_MockThreatDetectorResources));
}
