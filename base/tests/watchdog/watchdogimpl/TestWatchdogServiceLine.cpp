/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockIWatchdogRequest.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Common/PluginApiImpl/PluginResourceManagement.h>
#include <modules/Common/PluginCommunicationImpl/PluginProxy.h>
#include <modules/Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <watchdog/watchdogimpl/Watchdog.h>
#include <watchdog/watchdogimpl/WatchdogServiceLine.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

namespace
{
    class ProcessReplacement
    {
    public:
        ProcessReplacement(std::function<std::unique_ptr<Common::Process::IProcess>()> functor)
        {
            Common::ProcessImpl::ProcessFactory::instance().replaceCreator(functor);
        }
        ~ProcessReplacement() { Common::ProcessImpl::ProcessFactory::instance().restoreCreator(); }
    };

    class TestWatchdogServiceLine : public LogOffInitializedTests
    {
        IgnoreFilePermissions ignoreFilePermissions;
    public:
        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        TestWatchdogServiceLine() : m_context(Common::ZMQWrapperApi::createContext())
        {
            replacePluginIpc("inproc://watchdogservice.ipc");
        }
        ~TestWatchdogServiceLine() {}


        void replacePluginIpc(std::string ipc)
        {
            std::string pluginname =
                "plugins/" + watchdog::watchdogimpl::WatchdogServiceLine::WatchdogServiceLineName() + ".ipc";
            Common::ApplicationConfiguration::applicationConfiguration().setData(pluginname, ipc);
        }

        Common::PluginCommunicationImpl::PluginProxy getPluginProxyToTest()
        {
            std::string pluginName{ watchdog::watchdogimpl::WatchdogServiceLine::WatchdogServiceLineName() };
            auto requester = m_context->getRequester();
            Common::PluginApiImpl::PluginResourceManagement::setupRequester(*requester, pluginName, 5, 5);
            return Common::PluginCommunicationImpl::PluginProxy{ std::move(requester), pluginName };
        }
    };

    std::vector<std::string> getDummyPluginNames()
    {
        return {"DummyPlugin1", "DummyPlugin2"};
    }
} // namespace

TEST_F(TestWatchdogServiceLine, Construction) // NOLINT
{
    EXPECT_NO_THROW(watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames)); // NOLINT
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceThrowsExceptionIfNotWatchdogServiceIsAvailable) // NOLINT
{
    EXPECT_THROW(
        watchdog::watchdogimpl::WatchdogServiceLine::requestUpdateService(*m_context),
        watchdog::watchdogimpl::WatchdogServiceException);
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillIndirectlyTriggerSophosSplUpdate) // NOLINT
{
    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{ "restart", "sophos-spl-update.service" };
        EXPECT_CALL(*mockProcess, exec(HasSubstr("systemctl"), args)).Times(1);
        EXPECT_CALL(*mockProcess, waitUntilProcessEnds());
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("service triggered"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    watchdog::watchdogimpl::WatchdogServiceLine::requestUpdateService(*m_context);
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillThrowExceptionIfSophosUpdateFails) // NOLINT
{
    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{ "restart", "sophos-spl-update.service" };
        EXPECT_CALL(*mockProcess, exec(HasSubstr("systemctl"), args)).Times(1);
        EXPECT_CALL(*mockProcess, waitUntilProcessEnds());
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("failed to trigger service"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    EXPECT_THROW(
        watchdog::watchdogimpl::WatchdogServiceLine::requestUpdateService(*m_context),
        watchdog::watchdogimpl::UpdateServiceReportError);
}

TEST_F(TestWatchdogServiceLine, WatchdogServiceWillShouldIgnoreInvalidRequests) // NOLINT
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    std::string policyXmlFilePath("anypolicy.xml");

    EXPECT_CALL(*mockFileSystem, readFile(policyXmlFilePath)).WillOnce(Return("anypolicy"));

    auto pluginProxy = getPluginProxyToTest();
    pluginProxy.queueAction("", "NotValidAction","");
    pluginProxy.applyNewPolicy("", policyXmlFilePath);
    std::vector<Common::PluginApi::StatusInfo> returnedStatus = pluginProxy.getStatus();
    ASSERT_EQ(returnedStatus.size(), 1);
    EXPECT_EQ(returnedStatus.at(0).statusWithoutTimestampsXml, "");
    EXPECT_EQ(returnedStatus.at(0).statusXml, "");
}

TEST_F(TestWatchdogServiceLine, WatchdogTelemetryReturnsExpectedData) // NOLINT
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto pluginProxy = getPluginProxyToTest();
    EXPECT_EQ(pluginProxy.getTelemetry(),
              "{\"DummyPlugin1-unexpected-restarts\":0,\"DummyPlugin2-unexpected-restarts\":0}");
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillIndirectlyTriggerSophosSplUpdateWorksWithTheFactory) // NOLINT
{
    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{ "restart", "sophos-spl-update.service" };
        EXPECT_CALL(*mockProcess, exec(HasSubstr("systemctl"), args)).Times(1);
        EXPECT_CALL(*mockProcess, waitUntilProcessEnds());
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("service triggered"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    Tests::TempDir tempdir("/tmp");
    replacePluginIpc("ipc://" + tempdir.absPath("wdsl_checkfactory.ipc"));
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto requester = watchdog::watchdogimpl::factory().create();
    requester->requestUpdateService();
}

TEST_F(TestWatchdogServiceLine, mockTriggerUpdateUsingFactory) // NOLINT
{
    IWatchdogRequestReplacement replacement;
    Tests::TempDir tempdir("/tmp");
    replacePluginIpc("ipc://" + tempdir.absPath("wdsl_checkmock.ipc"));
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto requester = watchdog::watchdogimpl::factory().create();
    requester->requestUpdateService();
}

TEST_F(TestWatchdogServiceLine, mockTriggerUpdateUsingFactoryCanControlExceptions) // NOLINT
{
    IWatchdogRequestReplacement replacement(std::string{ "this was not expected" });
    Tests::TempDir tempdir("/tmp");
    replacePluginIpc("ipc://" + tempdir.absPath("wdsl_checkmock.ipc"));
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto requester = watchdog::watchdogimpl::factory().create();
    EXPECT_THROW(requester->requestUpdateService(), watchdog::watchdogimpl::WatchdogServiceException);
}
