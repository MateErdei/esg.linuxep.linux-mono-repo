/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/PluginCallback.h"
#include "pluginimpl/QueueTask.h"

#include <tests/common/LogInitializedTests.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <gtest/gtest.h>

#include <fstream>


#define BASE "/tmp/TestPluginCallback"

namespace fs = sophos_filesystem;

namespace
{
    class TestPluginCallback : public LogInitializedTests
    {
    public:
        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", BASE);
            fs::create_directories(BASE);
            fs::path versionFile = BASE;
            versionFile /= "VERSION.ini";

            std::ofstream ofs;
            ofs.open(versionFile);
            ofs << "PRODUCT_NAME = Sophos Server Protection Linux - av" << std::endl;
            ofs << "PRODUCT_VERSION = 1.2.3.456" << std::endl;
            ofs << "BUILD_DATE = 1970-00-01" << std::endl;
            ofs.close();
        };
    };
}

TEST_F(TestPluginCallback, getTelemetry) //NOLINT
{
    std::string expectedJson{ R"sophos({"version":"1.2.3.456"})sophos" };

    std::shared_ptr<Plugin::QueueTask> task = nullptr;
    Plugin::PluginCallback pluginCallback(task);
    EXPECT_EQ(pluginCallback.getTelemetry(), expectedJson);
}