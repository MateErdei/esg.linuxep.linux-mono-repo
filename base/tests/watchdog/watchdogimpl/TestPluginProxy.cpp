/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <watchdog/watchdogimpl/PluginProxy.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <modules/Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/ProcessImpl/MockProcess.h>


namespace
{
    class TestPluginProxy
            : public ::testing::Test
    {
    public:
    };
}

TEST_F(TestPluginProxy, TestConstruction) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo info;
    EXPECT_NO_THROW(watchdog::watchdogimpl::PluginProxy proxy(info));
}

TEST_F(TestPluginProxy, WontStartPluginWithoutExecutable) //NOLINT
{
    Common::PluginRegistryImpl::PluginInfo info;

    watchdog::watchdogimpl::PluginProxy proxy(info);
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay.count(),3600);
}

TEST_F(TestPluginProxy, WillStartPluginWithExecutable) //NOLINT
{
    const std::string INST=Common::ApplicationConfiguration::applicationPathManager().sophosInstall();;
    const std::string execPath = "./foobar";
    const std::string absolutePath = INST+"/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            [absolutePath](){
                std::vector<std::string> args;
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, exec(absolutePath, args, _)).Times(1);
                EXPECT_CALL(*mockProcess, kill()).Times(1); // In the destructor of PluginProxy
                EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
       }
    );

    Common::PluginRegistryImpl::PluginInfo info;
    info.setExecutableFullPath(execPath);

    watchdog::watchdogimpl::PluginProxy proxy(info);
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay,std::chrono::hours(1));
    Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
}

TEST_F(TestPluginProxy, WillWaitAfterExitBeforeRestartingPlugin) // NOLINT
{
    const std::string INST=Common::ApplicationConfiguration::applicationPathManager().sophosInstall();;
    const std::string execPath = "./foobar";
    const std::string absolutePath = INST+"/foobar";
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
            [absolutePath](){
                std::vector<std::string> args;
                auto mockProcess = new StrictMock<MockProcess>();
                EXPECT_CALL(*mockProcess, exec(absolutePath, args, _)).Times(1);
                EXPECT_CALL(*mockProcess, setOutputLimit(_)).Times(1);
                EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
                EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
                EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
                return std::unique_ptr<Common::Process::IProcess>(mockProcess);
            }
    );

    Common::PluginRegistryImpl::PluginInfo info;
    info.setExecutableFullPath(execPath);

    watchdog::watchdogimpl::PluginProxy proxy(info);
    std::chrono::seconds delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay,std::chrono::hours(1));

    EXPECT_NO_THROW(proxy.checkForExit()); //NOLINT
    delay = proxy.ensureStateMatchesOptions();
    EXPECT_EQ(delay,std::chrono::seconds(10)); // Not starting for 10 seconds

    Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
}
