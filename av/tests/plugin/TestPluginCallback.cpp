/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/sophos_filesystem.h"
#include "pluginimpl/PluginCallback.h"
#include "pluginimpl/QueueTask.h"

#include <tests/common/LogInitializedTests.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <gtest/gtest.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>

namespace fs = sophos_filesystem;
using json = nlohmann::json;

namespace
{
    class TestPluginCallback : public LogInitializedTests
    {
    public:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
            m_basePath = fs::temp_directory_path();
            m_basePath /= test_info->test_case_name();
            m_basePath /= test_info->name();
            fs::remove_all(m_basePath);
            fs::create_directories(m_basePath);

            //creating initial version file
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", m_basePath);
            m_versionFile = m_basePath;
            m_versionFile /= "VERSION.ini";

            std::ofstream versionFileStream;
            versionFileStream.open(m_versionFile);
            versionFileStream << "PRODUCT_NAME = Sophos Server Protection Linux - av" << std::endl;
            versionFileStream << "PRODUCT_VERSION = " << m_initialExpectedVersion << std::endl;
            versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
            versionFileStream.close();

            //creating initial ml library file
            fs::path libDirPath (m_basePath);
            libDirPath /= "chroot/susi/update_source/mllib";
            fs::create_directories(libDirPath);
            m_mlLibPath = libDirPath;
            m_mlLibPath /= "libmodel.so";

            std::ofstream mlFilePathStream;
            mlFilePathStream.open(m_mlLibPath);
            mlFilePathStream << "1"  << std::endl;
            mlFilePathStream.close();

            //creating initial lr data files
            fs::path lrDirPath (m_basePath);
            lrDirPath /= "chroot/susi/update_source/reputation";
            fs::create_directories(lrDirPath);
            m_lrFilerepPath = lrDirPath;
            m_lrFilerepPath /= "filerep.dat";
            m_lrSignerrepPath = lrDirPath;
            m_lrSignerrepPath /= "signerrep.dat";

            std::ofstream lrFilerepStream;
            lrFilerepStream.open(m_lrFilerepPath);
            lrFilerepStream << "1"  << std::endl;
            lrFilerepStream.close();

            std::ofstream lrSignerrepStream;
            lrSignerrepStream.open(m_lrSignerrepPath);
            lrSignerrepStream << "1"  << std::endl;
            lrSignerrepStream.close();

            //creating initial ml model file
            std::string initialHexString = "7374617469635f5f5f5f646574656374696f6e00ed03000004000000723b3401ec25b100010000000000000000001000";
            fs::path mlModelDirPath = m_basePath;
            mlModelDirPath /= "chroot/susi/update_source/model";
            fs::create_directories(mlModelDirPath);
            m_mlModelPath = mlModelDirPath / "model.dat";
            writeHexStringToFile(initialHexString, m_mlModelPath);

            //creating file for vdl version
            m_vdlDirPath = m_basePath;
            m_vdlDirPath /= "chroot/susi/update_source/vdl";
            fs::create_directories(m_vdlDirPath);
            m_vdlVersionFilePath = m_vdlDirPath;
            m_vdlVersionFilePath /= "vvf.xml";
            createVvfFile(m_initialExpectedVdlVersion,m_vdlVersionFilePath);

            //creating ide files
            createIdes(m_initialExpectedVdlIdeCount, m_vdlDirPath);

            std::shared_ptr<Plugin::QueueTask> task = nullptr;
            m_pluginCallback = std::make_shared<Plugin::PluginCallback>(task);
        };

        void TearDown() override
        {
            fs::remove_all(m_basePath);
        }

        void createIdes(const unsigned long &count, const fs::path &dirPath)
        {
            for (unsigned long i=0; i<count; ++i)
            {
                std::stringstream ideFileName;
                ideFileName << "test" << i << ".ide";
                fs::path ideFilePath = dirPath / ideFileName.str();
                std::ofstream ideFs(ideFilePath);
                ideFs.close();

                std::stringstream vdbFileName;
                vdbFileName << "test" << i << ".vdb";
                fs::path vdbFilePath = dirPath / vdbFileName.str();
                std::ofstream vdbFs(vdbFilePath);
                vdbFs.close();
            }
        }

        void createVvfFile(const std::string &version, const std::string &filePath)
        {
            std::string vvfContents = Common::UtilityImpl::StringUtils::orderedStringReplace(
                R"sophos(<?xml version='1.0' encoding='utf-8'?>
<VVF xmlns:nl="http://www.sophos.com/vdl/namelists" xmlns:sxt="http://www.sophos.com/vdl/sophxtainer" xmlns:vtyp="http://www.sophos.com/vdl/typesandsubtypes">
  <VirusData Version="@@VERSION@@" date="2020-09-08" />
  <sxt:sophxtainer>
    <sxt:section name="vdltypesandsubtypes">
    </sxt:section>
  </sxt:sophxtainer>
</VVF>)sophos",{
                    {"@@VERSION@@", version}
                });

            std::ofstream vdlVersionFileStream;
            vdlVersionFileStream.open(filePath);
            vdlVersionFileStream << vvfContents << std::endl;
            vdlVersionFileStream.close();
        }

        void writeHexStringToFile(const std::string &hexString, const fs::path &filePath)
        {
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

        fs::path m_basePath;
        fs::path m_lrFilerepPath;
        fs::path m_lrSignerrepPath;
        fs::path m_mlLibPath;
        fs::path m_mlModelPath;
        fs::path m_vdlDirPath;
        fs::path m_vdlVersionFilePath;
        fs::path m_versionFile;

        std::string m_initialExpectedLrHash = "ad0fadf63cc7cd779ce475e345bf4063565b63a3c2efef1eebc89790aaa6acba";
        std::string m_initialExpectedMlLibHash = "4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865";
        std::string m_initialExpectedMlModelVersion = "20200306";
        unsigned long m_initialExpectedVdlIdeCount = 3;
        std::string m_initialExpectedVdlVersion = "5.78";
        std::string m_initialExpectedVersion = "1.2.3.456";
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

TEST_F(TestPluginCallback, getTelemetry_version_fileDoesNotExist) //NOLINT
{
    fs::remove(m_versionFile);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["version"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_mlLibHash) //NOLINT
{
    std::string modifiedExpectedMlLibHash = "53c234e5e8472b6ac51c1ae1cab3fe06fad053beb8ebfd8977b010655bfdd3c3";

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["ml-lib-hash"], m_initialExpectedMlLibHash);

    std::ofstream mlFilePathStream;
    mlFilePathStream.open(m_mlLibPath);
    mlFilePathStream << "2"  << std::endl;
    mlFilePathStream.close();

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["ml-lib-hash"], modifiedExpectedMlLibHash);
}

TEST_F(TestPluginCallback, getTelemetry_mlLibHash_fileDoesNotExist) //NOLINT
{
    fs::remove(m_mlLibPath);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["ml-lib-hash"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_mlModelVersion) //NOLINT
{
    std::string modifiedHexString = "7374617469635f5f5f5f646574656374696f6e00ed03000004000000733b3401ec25b100010000000000000000001000";
    std::string modifiedExpectedMlModelVersion = "20200307";

    std::string initJson = m_pluginCallback->getTelemetry();
    json initialTelemetry = json::parse(initJson);

    EXPECT_EQ(initialTelemetry["ml-pe-model-version"], m_initialExpectedMlModelVersion);

    writeHexStringToFile(modifiedHexString, m_mlModelPath);

    std::string modJson = m_pluginCallback->getTelemetry();
    json modifiedTelemetry = json::parse(modJson);

    EXPECT_EQ(modifiedTelemetry["ml-pe-model-version"], modifiedExpectedMlModelVersion);
}

TEST_F(TestPluginCallback, getTelemetry_mlModelVersion_fileDoesNotExist) //NOLINT
{
    fs::remove(m_mlModelPath);

    std::string initJson = m_pluginCallback->getTelemetry();
    json telemetry = json::parse(initJson);

    EXPECT_EQ(telemetry["ml-pe-model-version"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_lrData) //NOLINT
{
    std::string modifiedExpectedLrHash = "8d58f634a5b5051db5ee0bb9d005779b382be410e8ebd5144590f2f0429f9220";

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["lr-data-hash"], m_initialExpectedLrHash);

    std::ofstream lrFilerepStream;
    lrFilerepStream.open(m_lrFilerepPath);
    lrFilerepStream << "2"  << std::endl;
    lrFilerepStream.close();

    std::ofstream lrSignerrepStream;
    lrSignerrepStream.open(m_lrSignerrepPath);
    lrSignerrepStream << "2"  << std::endl;
    lrSignerrepStream.close();

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["lr-data-hash"], modifiedExpectedLrHash);
}

TEST_F(TestPluginCallback, getTelemetry_lrData_fileDoesNotExist) //NOLINT
{
    fs::remove(m_lrFilerepPath);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["lr-data-hash"], "unknown");

    fs::remove(m_lrSignerrepPath);

    telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["lr-data-hash"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_vdlIdeCount) //NOLINT
{
    unsigned long modifiedExpectedVdlIdeCount = 4;

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["vdl-ide-count"], m_initialExpectedVdlIdeCount);

    createIdes(modifiedExpectedVdlIdeCount, m_vdlDirPath);

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["vdl-ide-count"], modifiedExpectedVdlIdeCount);
}

TEST_F(TestPluginCallback, getTelemetry_vdlIdeCount_dirDoesNotExist) //NOLINT
{
    fs::remove_all(m_vdlDirPath);

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["vdl-ide-count"], 0);
}

TEST_F(TestPluginCallback, getTelemetry_vdlVersion) //NOLINT
{
    std::string modifiedExpectedVdlVersion = "5.79";

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["vdl-version"], m_initialExpectedVdlVersion);

    createVvfFile(modifiedExpectedVdlVersion,m_vdlVersionFilePath);

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["vdl-version"], modifiedExpectedVdlVersion);
}

TEST_F(TestPluginCallback, getTelemetry_vdlVersion_fileDoesNotExist) //NOLINT
{
    fs::remove(m_vdlVersionFilePath);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["vdl-version"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_sxl4Lookup) //NOLINT
{
    m_pluginCallback->setSXL4Lookups(true);

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["sxl4-lookup"], true);

    m_pluginCallback->setSXL4Lookups(false);

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["sxl4-lookup"], false);
}

TEST_F(TestPluginCallback, getTelemetry_sxl4Lookup_fileDoesNotExist) //NOLINT
{
    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["sxl4-lookup"], true);
}

TEST_F(TestPluginCallback, getTelemetryHealthDefaultIsOne) //NOLINT
{
    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(modifiedTelemetry["health"], 1);
}

TEST_F(TestPluginCallback, getTelemetryHealthReturnsZeroWhenHealthSuccessfullyCalculated) //NOLINT
{
    m_pluginCallback->calculateHealth();
    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(modifiedTelemetry["health"], 0);
}

TEST_F(TestPluginCallback, getHealthReturnsZero) //NOLINT
{
    nlohmann::json j;
    j["Health"] = 0;
    std::string expectedResult = j.dump();
    std::string result = m_pluginCallback->getHealth();

    ASSERT_EQ(result, expectedResult);
}
