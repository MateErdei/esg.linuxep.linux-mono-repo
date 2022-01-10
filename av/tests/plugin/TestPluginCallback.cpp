/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "pluginimpl/PluginCallback.h"
#include "pluginimpl/QueueTask.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/MockFilePermissions.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <gtest/gtest.h>
#include <pluginimpl/HealthStatus.h>
#include <tests/common/LogInitializedTests.h>
#include <tests/datatypes/MockSysCalls.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>

using namespace Plugin;
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

TEST_F(TestPluginCallback, getTelemetry_threatHealth) //NOLINT
{
    m_pluginCallback->setThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["threatHealth"], E_THREAT_HEALTH_STATUS_GOOD);

    m_pluginCallback->setThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["threatHealth"], E_THREAT_HEALTH_STATUS_SUSPICIOUS);
}

TEST_F(TestPluginCallback, getTelemetry_threatHealth_fileDoesNotExist) //NOLINT
{
    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["threatHealth"], E_THREAT_HEALTH_STATUS_GOOD);
}

TEST_F(TestPluginCallback, getTelemetry_health) //NOLINT
{
    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    ASSERT_EQ(initialTelemetry["health"], E_HEALTH_STATUS_BAD);

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statusProcContents = "Uid: 5678";
    int uidContentsConverted = 5678;
    std::string expectedUsername = "sophos-spl-threat-detector";
    std::optional<std::string> cmdlineProcContents = "sophos_threat_detector\0";
    std::optional<std::string> statProcContents = "1 2 3";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillRepeatedly(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Return(expectedUsername));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "cmdline")).WillOnce(Return(cmdlineProcContents));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    ASSERT_EQ(modifiedTelemetry["health"], E_HEALTH_STATUS_GOOD);
}

TEST_F(TestPluginCallback, getHealthReturnsZeroWhenCalculateHealthSuccessful) //NOLINT
{
    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statusProcContents = "Uid: 5678";
    int uidContentsConverted = 5678;
    std::string expectedUsername = "sophos-spl-threat-detector";
    std::optional<std::string> cmdlineProcContents = "sophos_threat_detector\0";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Return(expectedUsername));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "cmdline")).WillOnce(Return(cmdlineProcContents));

    long expectedResult = E_HEALTH_STATUS_GOOD;
    long result = m_pluginCallback->calculateHealth();

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsZeroWhenShutdownFileIsValid) //NOLINT
{
    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, lastModifiedTime(shutdownFilePath)).WillOnce(Return(std::time(nullptr) - 7));

    long expectedResult = E_HEALTH_STATUS_GOOD;
    long result = m_pluginCallback->calculateHealth();

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsOneWhenPidFileDoesNotExistAndShutdownFileHasExpired) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, lastModifiedTime(shutdownFilePath)).WillOnce(Return(std::time(nullptr) - 20));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("File does not exist.")));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing threat detector pid file: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsOneWhenPidFileDoesNotExistAndShutdownHasAFileSystemException) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, lastModifiedTime(shutdownFilePath)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("Shutdown file read error.")));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("File does not exist.")));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing threat detector expected shutdown file: "));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing threat detector pid file: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfPidFileContentsAreMalformed) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string badPidContents = "notanint";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(badPidContents));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read Pid file to int due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfPidDirectoryInProcIsMissing) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(false));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos Threat Detector process no longer running: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfAccessingPidDirectoryHasAFileSystemException) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("Cannot check this directory.")));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing proc directory of pid: 1234 due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfProcStatusUidMalformed) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::string badProcStatusContents = "Uid: notanint";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(badProcStatusContents));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read Pid Status file to int due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfUidUsernameDoesNotCorrespondToSophosThreatDetector) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::string statusProcContents = "Uid: 5678";
    int uidContentsConverted = 5678;
    std::string unexpectedUsername = "some-other-username";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Return(unexpectedUsername));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unexpected user permissions for /proc/"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfProcStatusFileFailsToBeRead) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statusProcContents = std::nullopt;

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Status file of Pid: 1234 is empty. Returning bad health due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneOnFileSystemExceptionWhenGettingUsername) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::string statusProcContents = "Uid: 5678";
    int uidContentsConverted = 5678;

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("File does not exist.")));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed whilst validating user file permissions of /proc/1234 due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsOneWhenCmdlineArgOfPidIsNotSophosThreatDetector) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statusProcContents = "Uid: 5678";
    int uidContentsConverted = 5678;
    std::string expectedUsername = "sophos-spl-threat-detector";
    std::optional<std::string> wrongCmdlineProcContents = "not_sophos_threat_detector\0";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Return(expectedUsername));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "cmdline")).WillOnce(Return(wrongCmdlineProcContents));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("The proc cmdline for 1234 does not equal the expected value (sophos_threat_detector): "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneOnWhenCmdlineProcFileCannotBeRead) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statusProcContents = "Uid: 5678";
    int uidContentsConverted = 5678;
    std::string expectedUsername = "sophos-spl-threat-detector";
    std::optional<std::string> cmdlineProcContents = std::nullopt;

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Return(expectedUsername));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "cmdline")).WillOnce(Return(cmdlineProcContents));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Cmdline file of Pid: 1234 is empty. Returning bad health due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneOnFileSystemExceptionWhenAccessingCmdlineProcFile) //NOLINT
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statusProcContents = "Uid: 5678";
    int uidContentsConverted = 5678;
    std::string expectedUsername = "sophos-spl-threat-detector";

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Return(expectedUsername));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "cmdline")).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("File does not exist.")));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error reading threat detector cmdline proc file due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getTelemetry_ProductInfo)
{
    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    ASSERT_EQ(initialTelemetry["threatMemoryUsage"], 0);
    ASSERT_EQ(initialTelemetry["threatProcessAge"], 0);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path pidProcDirectory = "/proc/1234";

    std::string expectedUsername = "sophos-spl-threat-detector";
    std::string pidFileContents = "1234";

    std::optional<std::string> statusProcContents = "Uid: 5678";
    std::optional<std::string> cmdlineProcContents = "sophos_threat_detector\0";
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 100 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";

    int pidFileContentsConverted = 1234;
    int uidContentsConverted = 5678;

    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillRepeatedly(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "status")).WillOnce(Return(statusProcContents));
    EXPECT_CALL(*filePermissionsMock, getUserName(uidContentsConverted)).WillOnce(Return(expectedUsername));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "cmdline")).WillOnce(Return(cmdlineProcContents));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    long expectAgeValueToBeGreaterThan = 0;
    long expectedMemoryValue = 500;

    ASSERT_EQ(modifiedTelemetry["threatMemoryUsage"], expectedMemoryValue);
    ASSERT_NE(modifiedTelemetry["threatProcessAge"], expectAgeValueToBeGreaterThan);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesWhenPidFileDoesNotExist) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("File does not exist.")));

    std::pair<long, long> expectedResult(0, 0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing threat detector pid file: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfPidFileContentsAreMalformed) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string badPidContents = "notanint";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(badPidContents));

    std::pair<long, long> expectedResult(0, 0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read Pid file to int due to: "));
    ASSERT_EQ(result, expectedResult);
}


TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfPidDirectoryInProcIsMissing) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(false));

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos Threat Detector process no longer running: "));
    ASSERT_EQ(result, expectedResult);
}


TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfAccessingPidDirectoryHasAFileSystemException) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("Cannot check this directory.")));

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing proc directory of pid: 1234 due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfStatFileSizeIsIncorrect) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::string statProcContents = "1 2 3";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("The proc stat for 1234 is not of expected size"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsMemoryCorrectAgeZeroWithInvalidSysInfoCall) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 100 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";
    std::pair<int, long> sysCallUptime(-1, 0);

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    EXPECT_CALL(*sysCallsMock, getSystemUpTime()).WillOnce(Return(sysCallUptime));

    long expectedMemoryValue = 500;
    long expectedAgeValue = 0;
    std::pair<long, long> expectedResult(expectedMemoryValue,expectedAgeValue);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to retrieve system info, cant calculate process duration."));

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsCorrectValuesWhenSuccessful) //NOLINT
{
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 450 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";
    std::pair<int, long> sysCallUptime(0, 1000);

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    EXPECT_CALL(*sysCallsMock, getSystemUpTime()).WillOnce(Return(sysCallUptime));

    long expectedMemoryValue = 500;
    long expectedAgeValue = 550;
    std::pair<long, long> expectedResult(expectedMemoryValue,expectedAgeValue);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfProcStatFileFailsToBeRead) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = std::nullopt;

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Stat file of Pid: 1234 is empty. Cannot report on Threat Detector Process to Telemetry: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesOnFileSystemExceptionWhenAccessingStatProcFile) //NOLINT
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    auto filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("File does not exist.")));

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error reading threat detector stat proc file due to: "));
    ASSERT_EQ(result, expectedResult);
}
