/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <watchdog/watchdogimpl/PluginProxy.h>

namespace
{
    class TestPluginProxy : public ::testing::Test
    {
    public:
        ~TestPluginProxy() { Tests::restoreFileSystem(); }

    private:
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    };

    class PluginProxyExposePluginInfo : public watchdog::watchdogimpl::PluginProxy
    {
    public:
        explicit PluginProxyExposePluginInfo(Common::PluginRegistryImpl::PluginInfo info) :
            watchdog::watchdogimpl::PluginProxy(std::move(info))
        {
        }
        Common::PluginRegistryImpl::PluginInfo& getPluginInfoPublic()
        {
            return *dynamic_cast<Common::PluginRegistryImpl::PluginInfo*>(m_processInfo.get());
        }
    };
} // namespace

TEST_F(TestPluginProxy, TestConstruction) // NOLINT
{
    Common::PluginRegistryImpl::PluginInfo info;
    EXPECT_NO_THROW(watchdog::watchdogimpl::PluginProxy proxy(std::move(info))); // NOLINT
}

TEST_F(TestPluginProxy, TestPluginNameCanBeAccessedFromPluginProxy) // NOLINT
{
    Common::PluginRegistryImpl::PluginInfo info;
    std::string pluginNameExpected("PluginName");
    info.setPluginName(pluginNameExpected);
    std::unique_ptr<watchdog::watchdogimpl::PluginProxy> proxy;
    EXPECT_NO_THROW(proxy.reset(new watchdog::watchdogimpl::PluginProxy(std::move(info)))); // NOLINT
    std::string pluginNameFromProxy = proxy->name();
    EXPECT_EQ(pluginNameExpected, pluginNameFromProxy);
}

TEST_F(TestPluginProxy, TestPluginInfoCanBeUpdatedInPluginProxy) // NOLINT
{
    Common::PluginRegistryImpl::PluginInfo info;
    std::unique_ptr<PluginProxyExposePluginInfo> proxy;
    EXPECT_NO_THROW(proxy.reset(new PluginProxyExposePluginInfo(std::move(info)))); // NOLINT
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