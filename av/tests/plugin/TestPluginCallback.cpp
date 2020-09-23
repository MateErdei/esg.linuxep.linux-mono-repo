/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/PluginCallback.h"
#include "pluginimpl/QueueTask.h"

#include <tests/common/LogInitializedTests.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <gtest/gtest.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>


#define BASE "/tmp/TestPluginCallback"

namespace fs = sophos_filesystem;
using json = nlohmann::json;

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
            versionFileStream << "PRODUCT_VERSION = " << m_initialExpectedVersion << std::endl;
            versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
            versionFileStream.close();

            fs::path libDirPath (BASE);
            libDirPath /= "chroot/susi/distribution_version/version1";
            fs::create_directories(libDirPath);
            m_mlLibFilePath = libDirPath;
            m_mlLibFilePath /= "libmodel.so";

            std::ofstream mlFilePathStream;
            mlFilePathStream.open(m_mlLibFilePath);
            mlFilePathStream << "1"  << std::endl;
            mlFilePathStream.close();

            std::shared_ptr<Plugin::QueueTask> task = nullptr;
            m_pluginCallback = std::make_shared<Plugin::PluginCallback>(task);
        };


        std::shared_ptr<Plugin::PluginCallback> m_pluginCallback;
        fs::path m_versionFile;
        fs::path m_mlLibFilePath;
        std::string m_initialExpectedVersion = "1.2.3.456";
        std::string m_initialExpectedMlLibHash = "4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865";
    };
}

TEST_F(TestPluginCallback, getTelemetry_version) //NOLINT
{
    std::string modifiedVersion = "1.2.3.457";

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["version"], m_initialExpectedVersion);

    std::ofstream versionFileStream;
    versionFileStream.open(m_versionFile);
    versionFileStream << "PRODUCT_NAME = Sophos Server Protection Linux - av" << std::endl;
    versionFileStream << "PRODUCT_VERSION = " << modifiedVersion << std::endl;
    versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
    versionFileStream.close();

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["version"], modifiedVersion);
}

TEST_F(TestPluginCallback, getTelemetry_mlLibHash) //NOLINT
{
    std::string modifiedExpectedMlLibHash = "53c234e5e8472b6ac51c1ae1cab3fe06fad053beb8ebfd8977b010655bfdd3c3";

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["ml-lib-hash"], m_initialExpectedMlLibHash);

    std::ofstream mlFilePathStream;
    mlFilePathStream.open(m_mlLibFilePath);
    mlFilePathStream << "2"  << std::endl;
    mlFilePathStream.close();

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["ml-lib-hash"], modifiedExpectedMlLibHash);
}