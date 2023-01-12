// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "sophos_threat_detector/sophosthreatdetectorimpl/SophosThreatDetectorMain.h"

#include "MockThreatDetectorResources.h"

#include "common/MemoryAppender.h"

#include <gtest/gtest.h>


using namespace sspl::sophosthreatdetectorimpl;
using namespace ::testing;


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

            m_mockSystemCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();
            m_mockThreatDetectorResources = std::make_shared<NiceMock<MockThreatDetectorResources>>(m_testDir, m_mockSystemCallWrapper);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        const std::string m_pluginInstall = "/tmp/TestSophosThreatDetectorMain";
        Common::ApplicationConfiguration::IApplicationConfiguration& m_appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path m_testDir;
        std::shared_ptr<NiceMock<MockThreatDetectorResources>> m_mockThreatDetectorResources;
        std::shared_ptr<NiceMock<MockSystemCallWrapper>> m_mockSystemCallWrapper;
    };

}

TEST_F(TestSophosThreatDetectorMain, throwsIfChrootFails)
{
    EXPECT_CALL(*m_mockSystemCallWrapper, chroot(_)).WillOnce(Return(-1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_mockThreatDetectorResources);
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

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*m_mockSystemCallWrapper, cap_get_proc()).WillOnce(SetErrnoAndReturn(EINVAL, nullptr));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_mockThreatDetectorResources);
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

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*m_mockSystemCallWrapper, cap_clear(_)).WillOnce(SetErrnoAndReturn(EINVAL, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_mockThreatDetectorResources);
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

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*m_mockSystemCallWrapper, cap_set_proc(_)).WillOnce(SetErrnoAndReturn(EPERM, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_mockThreatDetectorResources);
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

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_)).WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSystemCallWrapper, getuid()).WillOnce(Return(0));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_mockThreatDetectorResources);

    EXPECT_TRUE(waitForLog("Running as root - Skip dropping of capabilities"));
}

TEST_F(TestSophosThreatDetectorMain, throwsIfprctlFails)
{
    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, getuid()).WillOnce(Return(1));
    EXPECT_CALL(*m_mockSystemCallWrapper, prctl(_,_,_,_,_)).WillOnce(SetErrnoAndReturn(EFAULT, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_mockThreatDetectorResources);
        FAIL() << "Threat detector main::chroot didnt throw";
    }
    catch (std::exception& ex)
    {
        EXPECT_STREQ(ex.what(), "Failed to lock capabilities after entering chroot: 14 (Bad address)");
    }
}

TEST_F(TestSophosThreatDetectorMain, throwsIfchdirFails)
{
    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, chdir(_)).WillOnce(SetErrnoAndReturn(EFAULT, -1));

    try
    {
        auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
        treatDetectorMain.inner_main(m_mockThreatDetectorResources);
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

    EXPECT_CALL(*m_mockThreatDetectorResources, createSigTermHandler(_)).WillOnce(Return(mockSigTermMonitor));
    EXPECT_CALL(*mockSigTermMonitor, triggered()).WillOnce(Return(true));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_mockThreatDetectorResources);

    ASSERT_TRUE(appenderContains("Starting updateCompleteNotifier"));
    EXPECT_TRUE(appenderContains("Sophos Threat Detector received SIGTERM - shutting down"));
    EXPECT_TRUE(appenderContains("Stopping updateCompleteNotifier"));
    EXPECT_TRUE(appenderContains("Joining updateCompleteNotifier"));
}


TEST_F(TestSophosThreatDetectorMain, logsWhenAttemptDNSQueryFailsWithEAI_SYSTEM)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, getaddrinfo(_,_,_,_)).WillOnce(SetErrnoAndReturn(EIO, EAI_SYSTEM));
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_)).WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_mockThreatDetectorResources);

    EXPECT_TRUE(appenderContains("Failed DNS query of 4.sophosxl.net: system error in getaddrinfo: Input/output error"));
}

TEST_F(TestSophosThreatDetectorMain, logsWhenAttemptDNSQueryFailsWithOtherError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, getaddrinfo(_,_,_,_)).WillOnce(Return(EAI_AGAIN));
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_)).WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    treatDetectorMain.inner_main(m_mockThreatDetectorResources);
    EXPECT_TRUE(appenderContains("Failed DNS query of 4.sophosxl.net: error in getaddrinfo: Temporary failure in name resolution"));
}

TEST_F(TestSophosThreatDetectorMain, logsShutdownTimeoutBeforePpoll)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto mockShutdownTimer = std::make_shared<NiceMock<MockShutdownTimer>>();

    EXPECT_CALL(*m_mockThreatDetectorResources, createShutdownTimer(_)).WillOnce(Return(mockShutdownTimer));

    {
        InSequence sequence;
        EXPECT_CALL(*mockShutdownTimer, timeout()).WillOnce(Return(10));
        EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_)).WillOnce(pollReturnsWithRevents(0, POLLIN));;
    }

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_CLEAN_SUCCESS, treatDetectorMain.inner_main(m_mockThreatDetectorResources));

    EXPECT_TRUE(appenderContains("Setting shutdown timeout to 10 seconds"));
}

TEST_F(TestSophosThreatDetectorMain, failureToUpdateScannerFactoryExitsInnerMain)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    auto mockScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();

    EXPECT_CALL(*m_mockThreatDetectorResources, createSusiScannerFactory(_,_,_)).WillOnce(Return(mockScannerFactory));
    EXPECT_CALL(*mockScannerFactory, update()).WillOnce(Return(false));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_GENERIC_FAILURE, treatDetectorMain.inner_main(m_mockThreatDetectorResources));

    EXPECT_TRUE(appenderContains("[FATAL] Update of scanner at startup failed exiting threat detector main"));
}

TEST_F(TestSophosThreatDetectorMain, continuesWhenReceivesErrorEINTR)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_))
        .WillOnce(SetErrnoAndReturn(EINTR, -1))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_CLEAN_SUCCESS, treatDetectorMain.inner_main(m_mockThreatDetectorResources));

    EXPECT_TRUE(appenderContains("Ignoring EINTR from ppoll"));
}

TEST_F(TestSophosThreatDetectorMain, exitsWhenPpollReturnsOtherError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_)).WillOnce(SetErrnoAndReturn(EACCES, -1));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_GENERIC_FAILURE, treatDetectorMain.inner_main(m_mockThreatDetectorResources));

    EXPECT_TRUE(appenderContains("Failed to poll signals. Error: "));
}

TEST_F(TestSophosThreatDetectorMain, timeoutExpiredWithInitialisedSusiResultsInQuickRestart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();
    auto mockShutdownTimer = std::make_shared<NiceMock<MockShutdownTimer>>();

    EXPECT_CALL(*mockScannerFactory, susiIsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(*mockShutdownTimer, timeout()).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_)).WillOnce(Return(0));

    EXPECT_CALL(*m_mockThreatDetectorResources, createShutdownTimer(_)).WillOnce(Return(mockShutdownTimer));
    EXPECT_CALL(*m_mockThreatDetectorResources, createSusiScannerFactory(_,_,_)).WillOnce(Return(mockScannerFactory));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_QUICK_RESTART_SUCCESS, treatDetectorMain.inner_main(m_mockThreatDetectorResources));

    EXPECT_TRUE(appenderContains("No scans requested for 0 seconds - shutting down."));
}

TEST_F(TestSophosThreatDetectorMain, timeoutWithValueAndInitialisedSusiDoesntQuickRestart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();
    auto mockShutdownTimer = std::make_shared<NiceMock<MockShutdownTimer>>();

    EXPECT_CALL(*mockScannerFactory, susiIsInitialized()).WillOnce(Return(true));
    EXPECT_CALL(*mockShutdownTimer, timeout()).WillRepeatedly(Return(5));
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_))
        .WillOnce(Return(0))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockThreatDetectorResources, createShutdownTimer(_)).WillOnce(Return(mockShutdownTimer));
    EXPECT_CALL(*m_mockThreatDetectorResources, createSusiScannerFactory(_,_,_)).WillOnce(Return(mockScannerFactory));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_CLEAN_SUCCESS, treatDetectorMain.inner_main(m_mockThreatDetectorResources));

    EXPECT_TRUE(appenderContains("Scan requested less than 5 seconds ago - continuing"));
}

TEST_F(TestSophosThreatDetectorMain, ppollTimeoutWithUninitialisedSusiResetsShutdownTimer)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();
    auto mockShutdownTimer = std::make_shared<NiceMock<MockShutdownTimer>>();

    EXPECT_CALL(*mockScannerFactory, susiIsInitialized()).WillOnce(Return(false));
    EXPECT_CALL(*mockShutdownTimer, timeout()).WillRepeatedly(Return(5));
    EXPECT_CALL(*mockShutdownTimer, reset()).Times(1);
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_))
        .WillOnce(Return(0))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockThreatDetectorResources, createShutdownTimer(_)).WillOnce(Return(mockShutdownTimer));
    EXPECT_CALL(*m_mockThreatDetectorResources, createSusiScannerFactory(_,_,_)).WillOnce(Return(mockScannerFactory));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_CLEAN_SUCCESS, treatDetectorMain.inner_main(m_mockThreatDetectorResources));

    EXPECT_TRUE(appenderContains("SUSI is not initialised - resetting shutdown timer"));
}




namespace {
    class TestPpollErrors :
            public TestSophosThreatDetectorMain,
            public testing::WithParamInterface<std::pair<int, int>>
    {};
}

INSTANTIATE_TEST_SUITE_P(
    TestSophosThreatDetectorMainPpoll,
    TestPpollErrors,
    testing::Values(
        std::make_pair(EFAULT, common::E_GENERIC_FAILURE),
        std::make_pair(EINTR, common::E_CLEAN_SUCCESS),
        std::make_pair(EINVAL, common::E_GENERIC_FAILURE),
        std::make_pair(ENOMEM, common::E_GENERIC_FAILURE),
        std::make_pair(EAGAIN, common::E_GENERIC_FAILURE)
                 ));

TEST_P(TestPpollErrors, innerMainReturnPpollError)
{
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_))
        .WillOnce(SetErrnoAndReturn(std::get<0>(GetParam()), -1))
        //Is will repeatedly because some params wont call this at all
        .WillRepeatedly(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(std::get<1>(GetParam()), treatDetectorMain.inner_main(m_mockThreatDetectorResources));
}


namespace {
    class TestFdEvents :
        public TestSophosThreatDetectorMain,
        public testing::WithParamInterface<std::tuple<int, int, int, std::string>>
    {};
}

constexpr int SIGTERM_MONITOR = 0;
constexpr int USR1_MONITOR = 1;
constexpr int SYSTEM_FILE_CHANGE = 2;

INSTANTIATE_TEST_SUITE_P(
    TestSophosThreatDetectorMainFdEvents,
    TestFdEvents,
    testing::Values(
        std::tuple(SIGTERM_MONITOR, POLLERR, common::E_GENERIC_FAILURE, "[FATAL] Error from sigterm pipe"),
        std::tuple(SIGTERM_MONITOR, POLLIN, common::E_CLEAN_SUCCESS, "[INFO] Sophos Threat Detector received SIGTERM - shutting down"),
        std::tuple(USR1_MONITOR, POLLERR, common::E_GENERIC_FAILURE, "[FATAL] Error from USR1 monitor"),
        std::tuple(USR1_MONITOR, POLLIN,  common::E_CLEAN_SUCCESS, "[INFO] Sophos Threat Detector received SIGUSR1 - reloading"),
        std::tuple(SYSTEM_FILE_CHANGE, POLLERR, common::E_GENERIC_FAILURE, "[FATAL] Error from system file change pipe"),
        std::tuple(SYSTEM_FILE_CHANGE, POLLIN,  common::E_QUICK_RESTART_SUCCESS, "[INFO] Sophos Threat Detector is restarting to pick up changed system files")
        ));


TEST_P(TestFdEvents, innerMainFdEvents)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    m_memoryAppender->setLayout(std::make_unique<log4cplus::PatternLayout>("[%p] %m%n"));

    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_))
        .WillOnce(pollReturnsWithRevents(std::get<0>(GetParam()), std::get<1>(GetParam())))
        //Is will repeatedly because some params wont call this at all
        .WillRepeatedly(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(std::get<2>(GetParam()), treatDetectorMain.inner_main(m_mockThreatDetectorResources));
    EXPECT_TRUE(appenderContains(std::get<3>(GetParam())));
}

TEST_F(TestSophosThreatDetectorMain, innerMainTriggersUsr1MonitorWhenReceivesUSR1)
{
    auto mockUSR1Monitor = std::make_shared<NiceMock<MockSignalHandler>>();

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockThreatDetectorResources, createUsr1Monitor(_)).WillOnce(Return(mockUSR1Monitor));
    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_))
        .WillOnce(pollReturnsWithRevents(USR1_MONITOR, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    //We need to trigger usr1Monitor as a result of ppoll
    EXPECT_CALL(*mockUSR1Monitor, triggered()).Times(1);

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_CLEAN_SUCCESS, treatDetectorMain.inner_main(m_mockThreatDetectorResources));
}

TEST_F(TestSophosThreatDetectorMain, innerMainTriggersSigTermHandlerWhenReceivesSigTerm)
{
    auto mockSigTermMonitor = std::make_shared<NiceMock<MockSignalHandler>>();

    EXPECT_CALL(*m_mockThreatDetectorResources, createSystemCallWrapper()).WillOnce(Return(m_mockSystemCallWrapper));
    EXPECT_CALL(*m_mockThreatDetectorResources, createSigTermHandler(_)).WillOnce(Return(mockSigTermMonitor));

    EXPECT_CALL(*m_mockSystemCallWrapper, ppoll(_,_,_,_))
        .WillOnce(pollReturnsWithRevents(SIGTERM_MONITOR, POLLIN));
    //We need to trigger sigterm monitor twice, once during startup and once as a result of ppoll
    EXPECT_CALL(*mockSigTermMonitor, triggered()).Times(2);

    auto treatDetectorMain = sspl::sophosthreatdetectorimpl::SophosThreatDetectorMain();
    EXPECT_EQ(common::E_CLEAN_SUCCESS, treatDetectorMain.inner_main(m_mockThreatDetectorResources));
}


