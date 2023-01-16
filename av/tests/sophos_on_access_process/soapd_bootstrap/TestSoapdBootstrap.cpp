// Copyright 2022-2023, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "../SoapMemoryAppenderUsingTests.h"
#include "common/MemoryAppender.h"
#include "datatypes/MockSysCalls.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "sophos_on_access_process/soapd_bootstrap/SoapdBootstrap.h"

#include <utility>

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
            ON_CALL(*m_systemCallWrapper, setrlimit(_, _)).WillByDefault(Return(0));
        }

        datatypes::ISystemCallWrapperSharedPtr getSystemCallWrapper() override
        {
            return m_systemCallWrapper;
        }

        IOnAccessServicePtr getOnAccessServiceImpl() override
        {
            return std::make_unique<MockOnAccessServiceImpl>();
        }

        std::shared_ptr<common::AbstractThreadPluginInterface> getUpdateClient(
            std::string socket_path,
            threat_scanner::IUpdateCompleteCallbackPtr callback
            ) override
        {
            return std::make_shared<DummyUpdateClient>(socket_path, callback);
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

        std::shared_ptr<MockSystemCallWrapper> m_systemCallWrapper;

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
    };
}

TEST_F(TestSoapdBootstrap, constructor)
{
    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
}

TEST_F(TestSoapdBootstrap, outerRun)
{
    expectSetupSyscalls();

    // Simulate SIGTERM
    EXPECT_CALL(*m_mockSoapdResources.m_systemCallWrapper, ppoll(_, _, _, _))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));

    auto soapdInstance = SoapdBootstrap(m_mockSoapdResources);
    auto ret = soapdInstance.outerRun();

    EXPECT_EQ(ret, 0);
}
