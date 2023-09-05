// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ProcessReplacement.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/PluginCommunicationImpl/PluginProxy.h"
#include "Common/WatchdogConstants/WatchdogConstants.h"
#include "Common/WatchdogRequest/IWatchdogRequest.h"
#include "Common/WatchdogRequest/WatchdogServiceException.h"
#include "Common/WatchdogRequestImpl/WatchdogRequestImpl.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockIWatchdogRequest.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "tests/Common/Helpers/TempDir.h"
#include "watchdog/watchdogimpl/WatchdogServiceLine.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>

namespace
{
    class TestWatchdogServiceLine : public LogOffInitializedTests
    {
        IgnoreFilePermissions ignoreFilePermissions;
    public:
        Common::ZMQWrapperApi::IContextSharedPtr m_context;
        TestWatchdogServiceLine() : m_context(Common::ZMQWrapperApi::createContext())
        {
            replacePluginIpc("inproc://watchdogservice.ipc");
        }
        ~TestWatchdogServiceLine() override = default;


        static void replacePluginIpc(const std::string& ipc)
        {
            std::string pluginname =
                "plugins/" + Common::WatchdogConstants::WatchdogServiceLineName() + ".ipc";
            Common::ApplicationConfiguration::applicationConfiguration().setData(pluginname, ipc);
        }

        Common::PluginCommunicationImpl::PluginProxy getPluginProxyToTest()
        {
            std::string pluginName{ Common::WatchdogConstants::WatchdogServiceLineName() };
            auto requester = m_context->getRequester();
            Common::PluginApiImpl::PluginResourceManagement::setupRequester(*requester, pluginName, 100, 10);
            return Common::PluginCommunicationImpl::PluginProxy{ std::move(requester), pluginName };
        }
    };

    std::vector<std::string> getDummyPluginNames()
    {
        return {"DummyPlugin1", "DummyPlugin2"};
    }
} // namespace

TEST_F(TestWatchdogServiceLine, Construction) 
{
    EXPECT_NO_THROW(watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames));
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceThrowsExceptionIfNotWatchdogServiceIsAvailable) 
{
    EXPECT_THROW(
        Common::WatchdogRequestImpl::requestUpdateService(*m_context),
        Common::WatchdogRequest::WatchdogServiceException);
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillIndirectlyTriggerSophosSplUpdate) 
{
    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{ "start", "sophos-spl-update.service" };
        EXPECT_CALL(*mockProcess, exec(HasSubstr("systemctl"), args)).Times(1);
        EXPECT_CALL(*mockProcess, waitUntilProcessEnds());
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("service triggered"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    Common::WatchdogRequestImpl::requestUpdateService(*m_context);
}

TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillThrowExceptionIfSophosUpdateFails) 
{
    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{ "start", "sophos-spl-update.service" };
        EXPECT_CALL(*mockProcess, exec(HasSubstr("systemctl"), args)).Times(1);
        EXPECT_CALL(*mockProcess, waitUntilProcessEnds());
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("failed to trigger service"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    EXPECT_THROW(
        Common::WatchdogRequestImpl::requestUpdateService(*m_context),
        Common::WatchdogRequest::UpdateServiceReportError);
}

TEST_F(TestWatchdogServiceLine, WatchdogServiceWillShouldIgnoreInvalidRequests) 
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, readFile(_)).Times(2).WillRepeatedly(Return("SomeContent"));

    auto pluginProxy = getPluginProxyToTest();
    pluginProxy.queueAction("", "NotValidAction.xml","");
    pluginProxy.applyNewPolicy("", "anypolicy.xml");
    std::vector<Common::PluginApi::StatusInfo> returnedStatus = pluginProxy.getStatus();
    ASSERT_EQ(returnedStatus.size(), 1);
    EXPECT_EQ(returnedStatus.at(0).statusWithoutTimestampsXml, "");
    EXPECT_EQ(returnedStatus.at(0).statusXml, "");
}

TEST_F(TestWatchdogServiceLine, WatchdogTelemetryReturnsExpectedData) 
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto pluginProxy = getPluginProxyToTest();

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isExecutable("/usr/bin/du")).WillOnce(Return(true));

    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{"-B", "1000", "-sx", "/opt/sophos-spl"};

        EXPECT_CALL(*mockProcess, exec(HasSubstr("du"), args)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, wait(Common::Process::Milliseconds(1000), 120)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("900000      /opt/sophos-spl"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_EQ(pluginProxy.getTelemetry(), "{\"DummyPlugin1-unexpected-restarts\":0,\"DummyPlugin2-unexpected-restarts\":0,\"health\":0,\"product-disk-usage\":900000}");
}

TEST_F(TestWatchdogServiceLine, WatchdogTelemetryReturnsExpectedDataWhenProductDiskUsageCommandFails) 
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto pluginProxy = getPluginProxyToTest();

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isExecutable("/usr/bin/du")).WillOnce(Return(true));

    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{"-B", "1000", "-sx", "/opt/sophos-spl"};

        EXPECT_CALL(*mockProcess, exec(HasSubstr("du"), args)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, wait(Common::Process::Milliseconds(1000), 120)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_EQ(pluginProxy.getTelemetry(), "{\"DummyPlugin1-unexpected-restarts\":0,\"DummyPlugin2-unexpected-restarts\":0,\"health\":0}");
}

TEST_F(TestWatchdogServiceLine, WatchdogTelemetryReturnsExpectedDataWhenProductDiskUsageCommandReturnsSingleString) 
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto pluginProxy = getPluginProxyToTest();

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isExecutable("/usr/bin/du")).WillOnce(Return(true));

    const std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    ProcessReplacement processReplacement([installPath]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{"-B", "1000", "-sx", installPath};

        EXPECT_CALL(*mockProcess, exec(HasSubstr("du"), args)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, wait(Common::Process::Milliseconds(1000), 120)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("blank"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_EQ(pluginProxy.getTelemetry(), "{\"DummyPlugin1-unexpected-restarts\":0,\"DummyPlugin2-unexpected-restarts\":0,\"health\":0}");
}

TEST_F(TestWatchdogServiceLine, WatchdogTelemetryReturnsExpectedDataWhenProductDiskUsageCommandReturnsString) 
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto pluginProxy = getPluginProxyToTest();

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isExecutable("/usr/bin/du")).WillOnce(Return(true));

    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{"-B", "1000", "-sx", "/opt/sophos-spl"};

        EXPECT_CALL(*mockProcess, exec(HasSubstr("du"), args)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, wait(Common::Process::Milliseconds(1000), 120)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("operation not permitted"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_EQ(pluginProxy.getTelemetry(), "{\"DummyPlugin1-unexpected-restarts\":0,\"DummyPlugin2-unexpected-restarts\":0,\"health\":0}");
}

TEST_F(TestWatchdogServiceLine, WatchdogTelemetryReturnsExpectedDataWhenProductDiskUsageCommandTimesOut) 
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto pluginProxy = getPluginProxyToTest();

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isExecutable("/usr/bin/du")).WillOnce(Return(true));

    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{"-B", "1000", "-sx", "/opt/sophos-spl"};

        EXPECT_CALL(*mockProcess, exec(HasSubstr("du"), args)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_));
        EXPECT_CALL(*mockProcess, wait(Common::Process::Milliseconds(1000), 120)).WillOnce(Return(Common::Process::ProcessStatus::TIMEOUT));
        EXPECT_CALL(*mockProcess, kill());
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_EQ(pluginProxy.getTelemetry(), "{\"DummyPlugin1-unexpected-restarts\":0,\"DummyPlugin2-unexpected-restarts\":0,\"health\":0}");
};

TEST_F(TestWatchdogServiceLine, WatchdogTelemetryReturnsExpectedDataWhenProductDiskUsageCommandCannotBeRan) 
{
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto pluginProxy = getPluginProxyToTest();

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isExecutable(_)).WillRepeatedly(Return(false));

    EXPECT_EQ(pluginProxy.getTelemetry(), "{\"DummyPlugin1-unexpected-restarts\":0,\"DummyPlugin2-unexpected-restarts\":0,\"health\":0}");
};


TEST_F(TestWatchdogServiceLine, requestUpdateServiceWillIndirectlyTriggerSophosSplUpdateWorksWithTheFactory) 
{
    ProcessReplacement processReplacement([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args{ "start", "sophos-spl-update.service" };
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
    auto requester = Common::WatchdogRequest::factory().create();
    requester->requestUpdateService();
}

TEST_F(TestWatchdogServiceLine, mockTriggerUpdateUsingFactory) 
{
    IWatchdogRequestReplacement replacement;
    Tests::TempDir tempdir("/tmp");
    replacePluginIpc("ipc://" + tempdir.absPath("wdsl_checkmock.ipc"));
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto requester = Common::WatchdogRequest::factory().create();
    requester->requestUpdateService();
}

TEST_F(TestWatchdogServiceLine, mockTriggerUpdateUsingFactoryCanControlExceptions) 
{
    IWatchdogRequestReplacement replacement(std::string{ "this was not expected" });
    Tests::TempDir tempdir("/tmp");
    replacePluginIpc("ipc://" + tempdir.absPath("wdsl_checkmock.ipc"));
    watchdog::watchdogimpl::WatchdogServiceLine WatchdogServiceLine(m_context, getDummyPluginNames);
    auto requester = Common::WatchdogRequest::factory().create();
    EXPECT_THROW(requester->requestUpdateService(), Common::WatchdogRequest::WatchdogServiceException);
}
