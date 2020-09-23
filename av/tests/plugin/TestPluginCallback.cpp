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

            fs::path mlDirPath (BASE);
            mlDirPath /= "chroot/susi/distribution_version/version1/mlmodel";
            fs::create_directories(mlDirPath);
            m_mlFilePath = mlDirPath;
            m_mlFilePath /= "model.dat";

            std::ofstream mlFilePathStream;
            mlFilePathStream.open(m_mlFilePath);
            mlFilePathStream << "1"  << std::endl;
            mlFilePathStream.close();

            std::string initialHexString = "7374617469635f5f5f5f646574656374696f6e00ed03000004000000723b3401ec25b100010000000000000000001000";
            m_mlLibPath = BASE;
            m_mlLibPath /= "chroot/susi/distribution_version/version1/libmodel.so";
            writeHexStringToFile(initialHexString, m_mlLibPath);

            std::shared_ptr<Plugin::QueueTask> task = nullptr;
            m_pluginCallback = std::make_shared<Plugin::PluginCallback>(task);
        };

        void writeHexStringToFile(const std::string &hexString, const fs::path &filePath){
            std::ofstream datafile(filePath, std::ios_base::binary | std::ios_base::out);

            char buf[3];
            buf[2] = 0;

            std::stringstream input(hexString);
            input.flags(std::ios_base::hex);
            while (input)
            {
                input >> buf[0] >> buf[1];
                long val = strtol(buf, nullptr, 16);
                datafile << static_cast<unsigned char>(val & 0xff);
            }
        }


        std::shared_ptr<Plugin::PluginCallback> m_pluginCallback;
        fs::path m_versionFile;
        fs::path m_mlFilePath;
        fs::path m_mlLibPath;
        std::string m_initialExpectedVersion = "1.2.3.456";
        std::string m_initialExpectedMlHash = "4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865";
        std::string m_initialExpectedMlModelVersion = "20200306";
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

TEST_F(TestPluginCallback, getTelemetry_mlData) //NOLINT
{
    std::string modifiedExpectedMlHash = "53c234e5e8472b6ac51c1ae1cab3fe06fad053beb8ebfd8977b010655bfdd3c3";

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["ml-pe-model-hash"], m_initialExpectedMlHash);

    std::ofstream mlFilePathStream;
    mlFilePathStream.open(m_mlFilePath);
    mlFilePathStream << "2"  << std::endl;
    mlFilePathStream.close();

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["ml-pe-model-hash"], modifiedExpectedMlHash);
}

TEST_F(TestPluginCallback, getTelemetry_mlModelVersion) //NOLINT
{
    std::string modifiedHexString = "7374617469635f5f5f5f646574656374696f6e00ed03000004000000733b3401ec25b100010000000000000000001000";
    std::string modifiedExpectedMlModelVersion = "20200307";

    std::string initJson = m_pluginCallback->getTelemetry();
    json initialTelemetry = json::parse(initJson);

    EXPECT_EQ(initialTelemetry["ml-pe-model-version"], m_initialExpectedMlModelVersion);

    writeHexStringToFile(modifiedHexString, m_mlLibPath);

    std::string modJson = m_pluginCallback->getTelemetry();
    json modifiedTelemetry = json::parse(modJson);

    EXPECT_EQ(modifiedTelemetry["ml-pe-model-version"], modifiedExpectedMlModelVersion);
}