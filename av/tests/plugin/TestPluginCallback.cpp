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
            m_versionFile = BASE;
            m_versionFile /= "VERSION.ini";

            std::ofstream versionFileStream;
            versionFileStream.open(m_versionFile);
            versionFileStream << "PRODUCT_NAME = Sophos Server Protection Linux - av" << std::endl;
            versionFileStream << "PRODUCT_VERSION = 1.2.3.456" << std::endl;
            versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
            versionFileStream.close();

            fs::path mlDirPath (BASE);
            mlDirPath /= "chroot/susi/distribution_version/version1/mlmodel";
            fs::create_directories(mlDirPath);
            m_mlFilePath = mlDirPath;
            m_mlFilePath /= "model.dat.0";

            std::ofstream mlFilePathStream;
            mlFilePathStream.open(m_mlFilePath);
            mlFilePathStream << "1"  << std::endl;
            mlFilePathStream.close();
        };

        fs::path m_versionFile;
        fs::path m_mlFilePath;
    };
}

TEST_F(TestPluginCallback, getTelemetry) //NOLINT
{
    // Note: telemetry dictionary appears to be alphabetically sorted
    std::string initialExpectedJson{ R"sophos({"ml-pe-model-hash":"4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865","version":"1.2.3.456"})sophos" };
    std::string modifiedExpectedJson{ R"sophos({"ml-pe-model-hash":"53c234e5e8472b6ac51c1ae1cab3fe06fad053beb8ebfd8977b010655bfdd3c3","version":"1.2.3.457"})sophos" };

    std::shared_ptr<Plugin::QueueTask> task = nullptr;
    Plugin::PluginCallback pluginCallback(task);

    std::string initialTelemetry = pluginCallback.getTelemetry();

    std::ofstream versionFileStream;
    versionFileStream.open(m_versionFile);
    versionFileStream << "PRODUCT_NAME = Sophos Server Protection Linux - av" << std::endl;
    versionFileStream << "PRODUCT_VERSION = 1.2.3.457" << std::endl;
    versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
    versionFileStream.close();

    std::ofstream mlFilePathStream;
    mlFilePathStream.open(m_mlFilePath);
    mlFilePathStream << "2"  << std::endl;
    mlFilePathStream.close();

    std::string modifiedTelemetry = pluginCallback.getTelemetry();

    EXPECT_EQ(initialTelemetry, initialExpectedJson);
    EXPECT_EQ(modifiedTelemetry, modifiedExpectedJson);
}