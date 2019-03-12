/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <watchdog/watchdogimpl/PluginProxy.h>

namespace
{
    class TestPluginProxy : public ::testing::Test
    {
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;

    public:
        TestPluginProxy() {}
    };
} // namespace

TEST_F(TestPluginProxy, TestConstruction) // NOLINT
{
    Common::PluginRegistryImpl::PluginInfo info;
    EXPECT_NO_THROW(watchdog::watchdogimpl::PluginProxy proxy(std::move(info))); // NOLINT
}

TEST_F(TestPluginProxy, WontStartPluginWithoutExecutable) // NOLINT
{
    Common::PluginRegistryImpl::PluginInfo info;

    watchdog::watchdogimpl::PluginProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay.count(), 3600);
}

TEST_F(TestPluginProxy, WillStartPluginWithExecutable) // NOLINT
{
    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, kill()).Times(1); // In the destructor of PluginProxy
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    Common::PluginRegistryImpl::PluginInfo info;
    info.setExecutableUserAndGroup("root:root");
    info.setExecutableFullPath(execPath);

    watchdog::watchdogimpl::PluginProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));
    Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
}

TEST_F(TestPluginProxy, WillWaitAfterExitBeforeRestartingPlugin) // NOLINT
{
    testing::internal::CaptureStderr();

    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    Common::PluginRegistryImpl::PluginInfo info;
    info.setExecutableUserAndGroup("root:root");
    info.setExecutableFullPath(execPath);

    watchdog::watchdogimpl::PluginProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));

    EXPECT_NO_THROW(proxy.checkForExit()); // NOLINT
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::seconds(10)); // Not starting for 10 seconds

    Common::ProcessImpl::ProcessFactory::instance().restoreCreator();

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::HasSubstr("/opt/sophos-spl/foobar exited when not expected"));
}

TEST_F(TestPluginProxy, checkExpectedExitIsNotLogged) // NOLINT
{
    testing::internal::CaptureStderr();

    const std::string INST = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    const std::string execPath = "./foobar";
    const std::string absolutePath = INST + "/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([absolutePath]() {
        std::vector<std::string> args;
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(absolutePath, args, _, _, _)).Times(1);
        EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
        EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(false));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    Common::PluginRegistryImpl::PluginInfo info;
    info.setExecutableUserAndGroup("root:root");
    info.setExecutableFullPath(execPath);

    watchdog::watchdogimpl::PluginProxy proxy(std::move(info));
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1));

    proxy.setEnabled(false);
    proxy.ensureStateMatchesOptions();

    EXPECT_NO_THROW(proxy.checkForExit()); // NOLINT
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay, std::chrono::hours(1)); // process has exited, so we

    Common::ProcessImpl::ProcessFactory::instance().restoreCreator();

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errStd, ::testing::Not(::testing::HasSubstr("/opt/sophos-spl/foobar exited when not expected")));
}