/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <watchdog/watchdogimpl/PluginProxy.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>


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
    time_t delay = proxy.startIfRequired();
    EXPECT_EQ(delay,3600);
}
