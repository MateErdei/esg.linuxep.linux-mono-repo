// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ProcessReplacement.h"

#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "watchdog/watchdogimpl/PluginProxy.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockProcess.h"

#include <nlohmann/json.hpp>

namespace
{
    class TestPluginProxy : public LogOffInitializedTests
    {
    public:
        void TearDown() override
        {
            Tests::restoreFileSystem();
        }

    private:
        IgnoreFilePermissions ignoreFilePermissions;
    };

    class PluginProxyExposePluginInfo : public watchdog::watchdogimpl::PluginProxy
    {
    public:
        using watchdog::watchdogimpl::PluginProxy::PluginProxy;
        
        Common::PluginRegistryImpl::PluginInfo& getPluginInfoPublic()
        {
            return *dynamic_cast<Common::PluginRegistryImpl::PluginInfo*>(m_processInfo.get());
        }
    };
} // namespace

TEST_F(TestPluginProxy, TestConstruction)
{
    Common::PluginRegistryImpl::PluginInfo info;
    EXPECT_NO_THROW(watchdog::watchdogimpl::PluginProxy proxy(std::move(info)));
}

TEST_F(TestPluginProxy, TestPluginNameCanBeAccessedFromPluginProxy)
{
    Common::PluginRegistryImpl::PluginInfo info;
    std::string pluginNameExpected("PluginName");
    info.setPluginName(pluginNameExpected);
    std::unique_ptr<watchdog::watchdogimpl::PluginProxy> proxy;
    EXPECT_NO_THROW(proxy.reset(new watchdog::watchdogimpl::PluginProxy(std::move(info))));
    std::string pluginNameFromProxy = proxy->name();
    EXPECT_EQ(pluginNameExpected, pluginNameFromProxy);
}

TEST_F(TestPluginProxy, TestPluginInfoCanBeUpdatedInPluginProxy)
{
    Common::PluginRegistryImpl::PluginInfo info;
    std::unique_ptr<PluginProxyExposePluginInfo> proxy;
    EXPECT_NO_THROW(proxy.reset(new PluginProxyExposePluginInfo(std::move(info))));
    auto& pluginProxyInfo = proxy->getPluginInfoPublic();

    EXPECT_EQ(pluginProxyInfo.getPluginName(), "");
    EXPECT_EQ(pluginProxyInfo.getXmlTranslatorPath(), "");
    EXPECT_EQ(pluginProxyInfo.getExecutableFullPath(), "");
    EXPECT_EQ(pluginProxyInfo.getExecutableArguments().empty(), true);
    EXPECT_EQ(pluginProxyInfo.getExecutableEnvironmentVariables().empty(), true);
    EXPECT_EQ(pluginProxyInfo.getPolicyAppIds().empty(), true);
    EXPECT_EQ(pluginProxyInfo.getStatusAppIds().empty(), true);
    EXPECT_EQ(pluginProxyInfo.getExecutableUserAndGroupAsString(), "");

    Common::PluginRegistryImpl::PluginInfo newInfo;
    newInfo.setPluginName("PluginName");
    newInfo.setXmlTranslatorPath("path1");
    newInfo.setExecutableFullPath("path2");
    newInfo.setExecutableArguments({ "arg1" });
    newInfo.addExecutableEnvironmentVariables("hello", "world");
    newInfo.setPolicyAppIds({ "app1" });
    newInfo.setStatusAppIds({ "app2" });
    newInfo.setExecutableUserAndGroup("user:group");

    proxy->updatePluginInfo(newInfo);

    EXPECT_EQ(pluginProxyInfo.getPluginName(), "PluginName");
    EXPECT_EQ(pluginProxyInfo.getXmlTranslatorPath(), "path1");
    EXPECT_EQ(pluginProxyInfo.getExecutableFullPath(), "path2");
    ASSERT_EQ(pluginProxyInfo.getExecutableArguments().size(), 1);
    EXPECT_EQ(pluginProxyInfo.getExecutableArguments().at(0), "arg1");
    ASSERT_EQ(pluginProxyInfo.getExecutableEnvironmentVariables().size(), 1);
    EXPECT_EQ(pluginProxyInfo.getExecutableEnvironmentVariables().at(0).first, "hello");
    EXPECT_EQ(pluginProxyInfo.getExecutableEnvironmentVariables().at(0).second, "world");
    ASSERT_EQ(pluginProxyInfo.getPolicyAppIds().size(), 1);
    EXPECT_EQ(pluginProxyInfo.getPolicyAppIds().at(0), "app1");
    ASSERT_EQ(pluginProxyInfo.getStatusAppIds().size(), 1);
    EXPECT_EQ(pluginProxyInfo.getStatusAppIds().at(0), "app2");
    EXPECT_EQ(pluginProxyInfo.getExecutableUserAndGroupAsString(), "user:group");
}

TEST_F(TestPluginProxy, checkForExitWithEmptyExecutable)
{
    Common::PluginRegistryImpl::PluginInfo info;
    auto proxy = std::make_unique<PluginProxyExposePluginInfo>(std::move(info));
    auto [waitPeriod, statusCode] = proxy->checkForExit();
    EXPECT_EQ(statusCode, Common::Process::ProcessStatus::FINISHED); // Empty executable gives FINISHED
}

TEST_F(TestPluginProxy, checkForExitWithRunningPlugin)
{
    ProcessReplacement processReplacement([]() {
                                              auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
                                              std::vector<std::string> args{ "60" };
                                              EXPECT_CALL(*mockProcess, exec("/bin/sleep", args, _, _, _)).Times(1);
                                              EXPECT_CALL(*mockProcess, setOutputLimit(_));
                                              EXPECT_CALL(*mockProcess, getStatus()).WillRepeatedly(Return(Common::Process::ProcessStatus::RUNNING));
                                              EXPECT_CALL(*mockProcess, kill(_)).Times(1);
                                              return mockProcess;
                                          });

    Common::PluginRegistryImpl::PluginInfo info;
    info.setExecutableFullPath("/bin/sleep");
    info.setExecutableArguments({"60"});
    auto proxy = std::make_unique<PluginProxyExposePluginInfo>(std::move(info));
    proxy->setEnabled(true);
    proxy->ensureStateMatchesOptions();
    {
        auto [waitPeriod, statusCode] = proxy->checkForExit();
        EXPECT_EQ(statusCode, Common::Process::ProcessStatus::RUNNING);
    }
    proxy.reset();
}


TEST_F(TestPluginProxy, checkForExitWithFinishedPlugin)
{
    ProcessReplacement processReplacement([]() {
                                              auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
                                              std::vector<std::string> args{};
                                              EXPECT_CALL(*mockProcess, exec("/bin/sleep", args, _, _, _)).Times(1);
                                              EXPECT_CALL(*mockProcess, setOutputLimit(_));
                                              EXPECT_CALL(*mockProcess, getStatus()).WillRepeatedly(Return(Common::Process::ProcessStatus::FINISHED));

                                              EXPECT_CALL(*mockProcess, exitCode()).WillRepeatedly(Return(42));
                                              EXPECT_CALL(*mockProcess, nativeExitCode()).WillRepeatedly(Return(42 << 8));
                                              EXPECT_CALL(*mockProcess, output()).WillRepeatedly(Return("xxx"));

                                              return mockProcess;
                                          });

    Common::PluginRegistryImpl::PluginInfo info;
    info.setExecutableFullPath("/bin/sleep");
    info.setPluginName("TestPlugin");
    auto proxy = std::make_unique<PluginProxyExposePluginInfo>(std::move(info));
    proxy->setEnabled(true);
    proxy->ensureStateMatchesOptions();
    {
        auto [waitPeriod, statusCode] = proxy->checkForExit();
        EXPECT_EQ(statusCode, Common::Process::ProcessStatus::FINISHED);
    }
    proxy.reset();

    auto telemetryString = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    nlohmann::json telemetry = nlohmann::json::parse(telemetryString);
    EXPECT_EQ(telemetry["TestPlugin-unexpected-restarts"], 1);
    EXPECT_EQ(telemetry["TestPlugin-unexpected-restarts-10752"], 1);
}