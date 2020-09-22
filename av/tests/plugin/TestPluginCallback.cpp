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

            fs::path lrDirPath (BASE);
            lrDirPath /= "chroot/susi/distribution_version/version1/lrdata";
            fs::create_directories(lrDirPath);
            m_lrFilerepPath = lrDirPath;
            m_lrFilerepPath /= "filerep.dat.0";
            m_lrSignerrepPath = lrDirPath;
            m_lrSignerrepPath /= "signerrep.dat.0";

            std::ofstream lrFilerepStream;
            lrFilerepStream.open(m_lrFilerepPath);
            lrFilerepStream << "1"  << std::endl;
            lrFilerepStream.close();

            std::ofstream lrSignerrepStream;
            lrSignerrepStream.open(m_lrSignerrepPath);
            lrSignerrepStream << "1"  << std::endl;
            lrSignerrepStream.close();
        };

        fs::path m_versionFile;
        fs::path m_mlFilePath;
        fs::path m_lrFilerepPath;
        fs::path m_lrSignerrepPath;
    };
}

TEST_F(TestPluginCallback, getTelemetry) //NOLINT
{
    // Note: telemetry dictionary appears to be alphabetically sorted
    std::string initialExpectedJson{ R"sophos({"lr-data-hash":"ad0fadf63cc7cd779ce475e345bf4063565b63a3c2efef1eebc89790aaa6acba","ml-pe-model-hash":"4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865","version":"1.2.3.456"})sophos" };
    std::string modifiedExpectedJson{ R"sophos({"lr-data-hash":"8d58f634a5b5051db5ee0bb9d005779b382be410e8ebd5144590f2f0429f9220","ml-pe-model-hash":"53c234e5e8472b6ac51c1ae1cab3fe06fad053beb8ebfd8977b010655bfdd3c3","version":"1.2.3.457"})sophos" };

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

    std::ofstream lrFilerepStream;
    lrFilerepStream.open(m_lrFilerepPath);
    lrFilerepStream << "2"  << std::endl;
    lrFilerepStream.close();

    std::ofstream lrSignerrepStream;
    lrSignerrepStream.open(m_lrSignerrepPath);
    lrSignerrepStream << "2"  << std::endl;
    lrSignerrepStream.close();

    std::string modifiedTelemetry = pluginCallback.getTelemetry();

    EXPECT_EQ(initialTelemetry, initialExpectedJson);
    EXPECT_EQ(modifiedTelemetry, modifiedExpectedJson);
}