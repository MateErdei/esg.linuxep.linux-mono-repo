// Copyright 2023 Sophos Limited. All rights reserved.

# define TEST_PUBLIC public

#include "pluginimpl/HealthStatus.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include "PluginMemoryAppenderUsingTests.h"

namespace
{
    class TestHealthStatus : public PluginMemoryAppenderUsingTests
    {
    public:
        void setPluginInstall(const char* pluginInstall="/opt/sophos-spl/plugins/av")
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", pluginInstall);
        }
    };
}

TEST_F(TestHealthStatus, absentFile)
{
    setPluginInstall();
    EXPECT_FALSE(Plugin::susiUpdateFailed());
}
