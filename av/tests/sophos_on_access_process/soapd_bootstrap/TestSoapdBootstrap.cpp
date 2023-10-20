// Copyright 2022-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "sophos_on_access_process/soapd_bootstrap/SoapdBootstrap.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <utility>

// test headers
#include "tests/sophos_on_access_process/SoapMemoryAppenderUsingTests.h"
#include "tests/common/MemoryAppender.h"
#include "tests/common/MockPidLock.h"
#include "tests/common/MockUpdateCompleteCallback.h"

#include "Common/Helpers/MockSysCalls.h"

#include <gtest/gtest.h>


using namespace sophos_on_access_process::soapd_bootstrap;
using namespace sophos_on_access_process::service_impl;
using namespace onaccessimpl::onaccesstelemetry;

namespace
{
    class MockTelemetryUtility : public IOnAccessTelemetryUtility
    {
    public:
        MOCK_METHOD(TelemetryEntry, getTelemetry, ());
        MOCK_METHOD(void, incrementEventReceived, (bool dropped));
        MOCK_METHOD(void, incrementFilesScanned, (bool error));
    };

    class MockOnAccessServiceImpl : public IOnAccessService
    {
    public:
        MockOnAccessServiceImpl()
        {
            m_telemetryUtility = std::make_shared<MockTelemetryUtility>();

            ON_CALL(*this, getTelemetryUtility()).WillByDefault(Return(m_telemetryUtility));
            EXPECT_CALL(*this, getTelemetryUtility());
        }

        MOCK_METHOD(IOnAccessTelemetryUtilitySharedPtr, getTelemetryUtility, ());

        IOnAccessTelemetryUtilitySharedPtr m_telemetryUtility;
    };

    class MockOnAccessRunner : public IOnAccessRunner
    {
    public:
        MOCK_METHOD(void, setupOnAccess, ());
        // these two methods are not thread safe, but are only called from ProcessPolicy and innerRun (after stopping
        // the policy handler)
        MOCK_METHOD(void, enableOnAccess, ());
        MOCK_METHOD(void, disableOnAccess, ());

        MOCK_METHOD(timespec*, getTimeout, ());
        MOCK_METHOD(void, onTimeout, ());

        MOCK_METHOD(threat_scanner::IUpdateCompleteCallbackPtr, getUpdateCompleteCallbackPtr, ());

        MOCK_METHOD(void, ProcessPolicy, ());
    };

    class DummyThread : public common::AbstractThreadPluginInterface
    {
    public:
        void run() override
        {
            announceThreadStarted();
        }
    };

    class DummyUpdateClient : public DummyThread
    {
    public:
        DummyUpdateClient(std::string socket_path, threat_scanner::IUpdateCompleteCallbackPtr callback) :
            m_socket_path(std::move(socket_path)), m_callback(std::move(callback))
        {
        }

        std::string m_socket_path;
        threat_scanner::IUpdateCompleteCallbackPtr m_callback;
    };

    class DummyProcessController : public DummyThread
    {
    public:
        DummyProcessController(
            std::string socketPath,
            std::string userName,
            std::string groupName,
            mode_t mode,
            std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallback) :
            m_socketPath(std::move(socketPath)),
            m_userName(std::move(userName)),
            m_groupName(std::move(groupName)),
            m_mode(mode),
            m_processControlCallback(std::move(processControlCallback))
        {
        }

        std::string m_socketPath;
        std::string m_userName;
        std::string m_groupName;
        mode_t m_mode;
        std::shared_ptr<unixsocket::IProcessControlMessageCallback> m_processControlCallback;

    };

    class MockedSoapdResources : public ISoapdResources
    {
    public:
        MockedSoapdResources()
        {
            m_systemCallWrapper = std::make_shared<MockSystemCallWrapper>();
            m_onAccessRunner = std::make_shared<MockOnAccessRunner>();
            ON_CALL(*m_systemCallWrapper, setrlimit(_, _)).WillByDefault(Return(0));
        }

        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  getSystemCallWrapper() override
        {
            return m_systemCallWrapper;
        }

        std::unique_ptr<common::IPidLockFile> getPidLockFile(const std::string& pidfile, bool changePidGroup) override
        {
            std::ignore = pidfile;
            std::ignore = changePidGroup;

            return std::make_unique<MockPidLock>();
        }

        IOnAccessServicePtr getOnAccessServiceImpl() override
        {
            return std::make_unique<MockOnAccessServiceImpl>();
        }

        std::shared_ptr<common::AbstractThreadPluginInterface> getUpdateClient(
            std::string socketPath,
            threat_scanner::IUpdateCompleteCallbackPtr callback
            ) override
        {
            return std::make_shared<DummyUpdateClient>(socketPath, callback);
        }

        std::shared_ptr<common::AbstractThreadPluginInterface> getProcessController(
            const std::string& socketPath,
            const std::string& userName,
            const std::string& groupName,
            mode_t mode,
            std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallback
            ) override
        {
            return std::make_shared<DummyProcessController>(
                socketPath, userName, groupName, mode, processControlCallback);
        }

        std::shared_ptr<IOnAccessRunner> getOnAccessRunner(
            Common::SystemCallWrapper::ISystemCallWrapperSharedPtr  /*sysCallWrapper*/,
            onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr /*telemetryUtility*/
            ) override
        {
            return m_onAccessRunner;
        }

        std::shared_ptr<MockSystemCallWrapper> m_systemCallWrapper;
        std::shared_ptr<MockOnAccessRunner> m_onAccessRunner;

    };

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
            // fs::create_directories(m_testDir / "var" / "ipc" / "plugins"); // for pluginApi ipc

            m_localSettings = fs::path(m_testDir / "var" / "on_access_local_settings.json");

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
            appConfig.setData("PLUGIN_INSTALL", m_testDir );

            m_updateCompleteCallback = std::make_shared<MockUpdateCompleteCallback>();
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        void expectSetupSyscalls()
        {
            EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, hardware_concurrency());
            EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, setrlimit(_, _));
        }

        fs::path m_testDir;
        fs::path m_localSettings;
        MockedSoapdResources m_mockSoapdResources;
        std::shared_ptr<MockUpdateCompleteCallback> m_updateCompleteCallback;
    };
}

TEST_F(TestSoapdBootstrap, constructor)
{
    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
}

TEST_F(TestSoapdBootstrap, outerRun_SIGINT)
{
    EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, ppoll(_, _, _, _))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, setupOnAccess());
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getUpdateCompleteCallbackPtr()).WillOnce(Return(m_updateCompleteCallback));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, ProcessPolicy());
    static timespec timeout { .tv_sec=60, .tv_nsec=0 };
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getTimeout()).WillOnce(Return(&timeout));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, disableOnAccess());

    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
    auto ret = soapdInstance.outerRun();

    EXPECT_EQ(ret, 0);
}

TEST_F(TestSoapdBootstrap, outerRun_SIGTERM)
{
    EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, ppoll(_, _, _, _))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, setupOnAccess());
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getUpdateCompleteCallbackPtr()).WillOnce(Return(m_updateCompleteCallback));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, ProcessPolicy());
    static timespec timeout { .tv_sec=60, .tv_nsec=0 };
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getTimeout()).WillOnce(Return(&timeout));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, disableOnAccess());

    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
    auto ret = soapdInstance.outerRun();

    EXPECT_EQ(ret, 0);
}

TEST_F(TestSoapdBootstrap, outerRun_timeout)
{
    EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, ppoll(_, _, _, _))
        .WillOnce(Return(0))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, setupOnAccess());
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getUpdateCompleteCallbackPtr()).WillOnce(Return(m_updateCompleteCallback));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, ProcessPolicy());
    static timespec timeout { .tv_sec=60, .tv_nsec=0 };
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getTimeout()).WillRepeatedly(Return(&timeout));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, disableOnAccess());
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, onTimeout());

    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
    auto ret = soapdInstance.outerRun();

    EXPECT_EQ(ret, 0);
}

TEST_F(TestSoapdBootstrap, outerRun_ppollFailsWithEINTR)
{
    EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, ppoll(_, _, _, _))
        .WillOnce(SetErrnoAndReturn(EINTR, -1))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, setupOnAccess());
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getUpdateCompleteCallbackPtr()).WillOnce(Return(m_updateCompleteCallback));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, ProcessPolicy());
    static timespec timeout { .tv_sec=60, .tv_nsec=0 };
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getTimeout()).WillRepeatedly(Return(&timeout));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, disableOnAccess());

    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
    auto ret = soapdInstance.outerRun();

    EXPECT_EQ(ret, 0);
}

TEST_F(TestSoapdBootstrap, outerRun_ppollFailsWithErrorOtherThanEINTR)
{
    EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, ppoll(_, _, _, _))
        .WillOnce(SetErrnoAndReturn(EBADF, -1));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, setupOnAccess());
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getUpdateCompleteCallbackPtr()).WillOnce(Return(m_updateCompleteCallback));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, ProcessPolicy());
    static timespec timeout { .tv_sec=60, .tv_nsec=0 };
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, getTimeout()).WillRepeatedly(Return(&timeout));
    EXPECT_CALL(*m_mockSoapdResources.m_onAccessRunner, disableOnAccess());

    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
    auto ret = soapdInstance.outerRun();

    EXPECT_EQ(ret, 0);
}