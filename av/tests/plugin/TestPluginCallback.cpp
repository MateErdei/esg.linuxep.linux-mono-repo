// Copyright 2020-2022, Sophos Limited.  All rights reserved.

# define TEST_PUBLIC public

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
#include <tests/common/MemoryAppender.h>
#include <tests/datatypes/MockSysCalls.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>

using namespace Plugin;
namespace fs = sophos_filesystem;
using json = nlohmann::json;

namespace
{
    class TestPluginCallback : public MemoryAppenderUsingTests
    {
    public:
        TestPluginCallback()
                : MemoryAppenderUsingTests("av")
        {}

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
            versionFileStream << "PRODUCT_NAME = SPL-Anti-Virus-Plugin" << std::endl;
            versionFileStream << "PRODUCT_VERSION = " << m_initialExpectedVersion << std::endl;
            versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
            versionFileStream << "COMMIT_HASH = " << m_initialExpectedCommitHash << std::endl;
            versionFileStream << "PLUGIN_API_COMMIT_HASH = " << m_initialExpectedPluginApiCommitHash << std::endl;
            versionFileStream.close();

            // create var directories for PID files
            fs::create_directories(m_basePath / "var");
            fs::create_directories(m_basePath / "chroot/var");

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
            m_vdlVersionFilePath /= "manifestdata.dat";
            createDataSetAFile(m_vdlVersionFilePath);

            //creating ide files
            createIdes(m_initialExpectedVdlIdeCount, m_vdlDirPath);

            std::shared_ptr<Plugin::QueueTask> task = nullptr;
            m_pluginCallback = std::make_shared<Plugin::PluginCallback>(task);
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            m_sysCalls = std::make_shared<datatypes::SystemCallWrapper>();
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

        void createDataSetAFile(const std::string& path)
        {
            std::ofstream vdlVersionFileStream;
            vdlVersionFileStream.open(path);
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
                assert(val >= 0);
                datafile << static_cast<unsigned char>(static_cast<unsigned>(val) & 0xff);
            }
        }

        std::shared_ptr<Plugin::PluginCallback> m_pluginCallback;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCalls;
        std::shared_ptr<datatypes::SystemCallWrapper> m_sysCalls;

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
        std::string m_initialExpectedVersion = "1.2.3.456";
        std::string m_initialExpectedCommitHash = "dd9f7b10bb62c4b5f8eee1fede4bb4f4100a75c5";
        std::string m_initialExpectedPluginApiCommitHash = "93b8ec8736dcb5b4266f85b1b08110ebe19c7f03";
    };
}

TEST_F(TestPluginCallback, getTelemetry_version)
{
    std::string modifiedVersion = "1.2.3.457";

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["version"], m_initialExpectedVersion);

    std::ofstream versionFileStream;
    versionFileStream.open(m_versionFile);
    versionFileStream << "PRODUCT_NAME = SPL-Anti-Virus-Plugin" << std::endl;
    versionFileStream << "PRODUCT_VERSION = " << modifiedVersion << std::endl;
    versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
    versionFileStream << "COMMIT_HASH = " << m_initialExpectedCommitHash << std::endl;
    versionFileStream << "PLUGIN_API_COMMIT_HASH = " << m_initialExpectedPluginApiCommitHash << std::endl;
    versionFileStream.close();

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["version"], modifiedVersion);
}

TEST_F(TestPluginCallback, getTelemetry_version_fileDoesNotExist)
{
    fs::remove(m_versionFile);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["version"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_mlLibHash)
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

TEST_F(TestPluginCallback, getTelemetry_mlLibHash_fileDoesNotExist)
{
    fs::remove(m_mlLibPath);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["ml-lib-hash"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_mlModelVersion)
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

TEST_F(TestPluginCallback, getTelemetry_mlModelVersion_fileDoesNotExist)
{
    fs::remove(m_mlModelPath);

    std::string initJson = m_pluginCallback->getTelemetry();
    json telemetry = json::parse(initJson);

    EXPECT_EQ(telemetry["ml-pe-model-version"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_lrData)
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

TEST_F(TestPluginCallback, getTelemetry_lrData_fileDoesNotExist)
{
    fs::remove(m_lrFilerepPath);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["lr-data-hash"], "unknown");

    fs::remove(m_lrSignerrepPath);

    telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["lr-data-hash"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_vdlIdeCount)
{
    unsigned long modifiedExpectedVdlIdeCount = 4;

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["vdl-ide-count"], m_initialExpectedVdlIdeCount);

    createIdes(modifiedExpectedVdlIdeCount, m_vdlDirPath);

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["vdl-ide-count"], modifiedExpectedVdlIdeCount);
}

TEST_F(TestPluginCallback, getTelemetry_vdlIdeCount_dirDoesNotExist)
{
    fs::remove_all(m_vdlDirPath);

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["vdl-ide-count"], 0);
}

TEST_F(TestPluginCallback, getTelemetry_vdlVersion)
{
    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["vdl-version"], "DataSet-A");
}

TEST_F(TestPluginCallback, getTelemetry_vdlVersion_fileDoesNotExist)
{
    fs::remove(m_vdlVersionFilePath);

    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["vdl-version"], "unknown");
}

TEST_F(TestPluginCallback, getTelemetry_sxl4Lookup)
{
    m_pluginCallback->setSXL4Lookups(true);

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(initialTelemetry["sxl4-lookup"], true);

    m_pluginCallback->setSXL4Lookups(false);

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(modifiedTelemetry["sxl4-lookup"], false);
}

TEST_F(TestPluginCallback, getTelemetry_sxl4Lookup_fileDoesNotExist)
{
    json telemetry = json::parse(m_pluginCallback->getTelemetry());

    EXPECT_EQ(telemetry["sxl4-lookup"], true);
}

TEST_F(TestPluginCallback, getTelemetry_threatHealth)
{
    UsingMemoryAppender memAppend(*this);

    m_pluginCallback->setThreatHealth(E_THREAT_HEALTH_STATUS_GOOD);

    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(initialTelemetry["threatHealth"], E_THREAT_HEALTH_STATUS_GOOD);

    m_pluginCallback->setThreatHealth(E_THREAT_HEALTH_STATUS_SUSPICIOUS);

    EXPECT_TRUE(appenderContains("Threat health changed to suspicious"));

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(modifiedTelemetry["threatHealth"], E_THREAT_HEALTH_STATUS_SUSPICIOUS);
}

TEST_F(TestPluginCallback, getTelemetry_threatHealth_fileDoesNotExist)
{
    json telemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(telemetry["threatHealth"], E_THREAT_HEALTH_STATUS_GOOD);
}

TEST_F(TestPluginCallback, getHealthReturnsZeroWhenPidfileExistsButIsNotLocked)
{
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::ofstream(threatDetectorPidFile).close();
    Path soapdPidFile = m_basePath / "var/soapd.pid";
    std::ofstream(soapdPidFile).close();

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_sysCalls);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsOneWhenPidFileDoesNotExistAndShutdownFileHasExpired)
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, lastModifiedTime(shutdownFilePath)).WillOnce(Return(std::time(nullptr) - 60));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_sysCalls);

    std::string logMessage = testing::internal::GetCapturedStderr();
    std::stringstream errMsg;
    errMsg << "Unable to open PID file " << threatDetectorPidFile << " (No such file or directory), assume process not running";
    EXPECT_THAT(logMessage, ::testing::HasSubstr(errMsg.str()));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsOneWhenPidFileDoesNotExistAndShutdownHasAFileSystemException)
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, lastModifiedTime(shutdownFilePath)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("Shutdown file read error.")));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_sysCalls);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing threat detector expected shutdown file: "));
    std::stringstream errMsg;
    errMsg << "Unable to open PID file " << threatDetectorPidFile << " (No such file or directory), assume process not running";
    EXPECT_THAT(logMessage, ::testing::HasSubstr(errMsg.str()));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsZeroIfLockCanBeTakenOnPidFiles)
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    int fileDescriptor = 123;
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    struct flock fl;
    fl.l_type    = F_WRLCK;
    EXPECT_CALL(*m_mockSysCalls, fcntl(fileDescriptor, F_GETLK, _)).WillRepeatedly(DoAll(SetArgPointee<2>(fl), Return(0)));

    long expectedResult = E_HEALTH_STATUS_GOOD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfLockCanBeTakenOnThreatDetectorPidFile)
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, lastModifiedTime(shutdownFilePath)).WillOnce(Throw(
        Common::FileSystem::IFileSystemException("Shutdown file read error.")));
    int fileDescriptor = 123;
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    struct flock threatDetectorFlock;
    threatDetectorFlock.l_type    = F_UNLCK;
    struct flock soapdFlock;
    soapdFlock.l_type    = F_RDLCK;
    struct flock safestoreFLock;
    safestoreFLock.l_type    = F_RDLCK;
    EXPECT_CALL(*m_mockSysCalls, fcntl(fileDescriptor, F_GETLK, _))
        .WillOnce(DoAll(SetArgPointee<2>(threatDetectorFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(soapdFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(safestoreFLock), Return(0)));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Lock acquired on PID file "));
    EXPECT_THAT(logMessage, ::testing::HasSubstr(" assume process not running"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos Threat Detector Process is not running, turning service health to red"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Service Health has changed to: red"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfLockCanBeTakenOnSoapdPidFile)
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    int fileDescriptor = 123;
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    struct flock threatDetectorFlock;
    threatDetectorFlock.l_type    = F_RDLCK;
    struct flock soapdFlock;
    soapdFlock.l_type    = F_UNLCK;
    struct flock safestoreFLock;
    safestoreFLock.l_type    = F_RDLCK;
    EXPECT_CALL(*m_mockSysCalls, fcntl(fileDescriptor, F_GETLK, _))
        .WillOnce(DoAll(SetArgPointee<2>(threatDetectorFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(soapdFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(safestoreFLock), Return(0)));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Lock acquired on PID file "));
    EXPECT_THAT(logMessage, ::testing::HasSubstr(" assume process not running"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos On Access Process is not running, turning service health to red"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Service Health has changed to: red"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsOneIfLockCanBeTakenOnSafestorePidFile)
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    int fileDescriptor = 123;
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    struct flock threatDetectorFlock;
    threatDetectorFlock.l_type    = F_RDLCK;
    struct flock soapdFlock;
    soapdFlock.l_type    = F_RDLCK;
    struct flock safestoreFLock;
    safestoreFLock.l_type    = F_UNLCK;
    EXPECT_CALL(*m_mockSysCalls, fcntl(fileDescriptor, F_GETLK, _))
        .WillOnce(DoAll(SetArgPointee<2>(threatDetectorFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(soapdFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(safestoreFLock), Return(0)));

    long expectedResult = E_HEALTH_STATUS_BAD;
    m_pluginCallback->setSafeStoreEnabled(true);
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Lock acquired on PID file "));
    EXPECT_THAT(logMessage, ::testing::HasSubstr(" assume process not running"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos Safestore Process is not running, turning service health to red"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Service Health has changed to: red"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getTelemetry_ProductInfo)
{
    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    Path soapdPidFile = m_basePath / "var/soapd.pid";

    ASSERT_EQ(initialTelemetry["threatMemoryUsage"], 0);
    ASSERT_EQ(initialTelemetry["threatProcessAge"], 0);

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    std::string threatDetectorPidFileContents = "1234";
    std::string soapdPidFileContents = "1235";
    int threatDetectorPidFileContentsConverted = 1234;
    Path threatDetectorPidProcDirectory = "/proc/1234";
    Path soapdPidProcDirectory = "/proc/1235";
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 100 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillRepeatedly(Return(threatDetectorPidFileContents));
    EXPECT_CALL(*filesystemMock, readFile(soapdPidFile)).WillRepeatedly(Return(soapdPidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(threatDetectorPidProcDirectory)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isDirectory(soapdPidProcDirectory)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(threatDetectorPidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    long expectAgeValueToBeGreaterThan = 0;
    long expectedMemoryValue = 500;

    ASSERT_EQ(modifiedTelemetry["threatMemoryUsage"], expectedMemoryValue);
    ASSERT_NE(modifiedTelemetry["threatProcessAge"], expectAgeValueToBeGreaterThan);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesWhenPidFileDoesNotExist)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("File does not exist.")));

    std::pair<long, long> expectedResult(0, 0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error accessing pid file: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfPidFileContentsAreMalformed)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string badPidContents = "notanint";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(badPidContents));

    std::pair<long, long> expectedResult(0, 0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read Pid file to int due to: "));
    ASSERT_EQ(result, expectedResult);
}


TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfPidDirectoryInProcIsMissing)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(false));

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Process no longer running: "));
    ASSERT_EQ(result, expectedResult);
}


TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfAccessingPidDirectoryHasAFileSystemException)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
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

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfStatFileSizeIsIncorrect)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::string statProcContents = "1 2 3";

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
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

TEST_F(TestPluginCallback, getProcessInfoReturnsMemoryCorrectAgeZeroWithInvalidSysInfoCall)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 100 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";
    std::pair<int, long> sysCallUptime(-1, 0);

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    auto* filesystemMock = new StrictMock<MockFileSystem>();
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

TEST_F(TestPluginCallback, getProcessInfoReturnsCorrectValuesWhenSuccessful)
{
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 450 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";
    std::pair<int, long> sysCallUptime(0, 1000);

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*filesystemMock, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    EXPECT_CALL(*sysCallsMock, getSystemUpTime()).WillOnce(Return(sysCallUptime));

    long expectedMemoryValue = 500;
    long expectedAgeValue = sysCallUptime.second - (450/sysconf(_SC_CLK_TCK));
    std::pair<long, long> expectedResult(expectedMemoryValue,expectedAgeValue);
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(sysCallsMock);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesIfProcStatFileFailsToBeRead)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = std::nullopt;

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
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

TEST_F(TestPluginCallback, getProcessInfoReturnsZeroesOnFileSystemExceptionWhenAccessingStatProcFile)
{
    testing::internal::CaptureStderr();

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;

    auto sysCallsMock = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
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

TEST_F(TestPluginCallback, checkCalculateServiceHealthLogsTheRightThings)
{
    testing::internal::CaptureStderr();

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    EXPECT_CALL(*filesystemMock, exists(shutdownFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, lastModifiedTime(shutdownFilePath)).WillRepeatedly(Throw(
        Common::FileSystem::IFileSystemException("Shutdown file read error.")));

    int fileDescriptor = 123;
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    struct flock threatDetectorFlock;
    threatDetectorFlock.l_type    = F_UNLCK;
    struct flock soapdFlock;
    soapdFlock.l_type    = F_UNLCK;
    struct flock safestoreFLock;
    safestoreFLock.l_type    = F_UNLCK;
    EXPECT_CALL(*m_mockSysCalls, fcntl(fileDescriptor, F_GETLK, _))
        .WillOnce(DoAll(SetArgPointee<2>(threatDetectorFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(soapdFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(safestoreFLock), Return(0)));

    m_pluginCallback->setSafeStoreEnabled(true);
    m_pluginCallback->m_safestoreServiceStatus = E_HEALTH_STATUS_BAD;
    m_pluginCallback->m_soapServiceStatus = E_HEALTH_STATUS_BAD;
    m_pluginCallback->m_threatDetectorServiceStatus = E_HEALTH_STATUS_BAD;
    m_pluginCallback->m_serviceHealth = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Lock acquired on PID file "));
    EXPECT_THAT(logMessage, ::testing::HasSubstr(" assume process not running"));
    EXPECT_THAT(logMessage, Not(::testing::HasSubstr("Sophos Safestore Process is not running, turning service health to red")));
    EXPECT_THAT(logMessage, Not(::testing::HasSubstr("Sophos On Access Process is not running, turning service health to red")));
    EXPECT_THAT(logMessage, Not(::testing::HasSubstr("Sophos Threat Detector Process is not running, turning service health to red")));
    EXPECT_THAT(logMessage, Not(::testing::HasSubstr("Service Health has changed to")));
    ASSERT_EQ(result, E_HEALTH_STATUS_BAD);

    //Now test that we log that service health has been reset
    testing::internal::CaptureStderr();

    threatDetectorFlock.l_type    = F_RDLCK;
    soapdFlock.l_type    = F_RDLCK;
    safestoreFLock.l_type    = F_RDLCK;
    EXPECT_CALL(*m_mockSysCalls, fcntl(fileDescriptor, F_GETLK, _))
        .WillOnce(DoAll(SetArgPointee<2>(threatDetectorFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(soapdFlock), Return(0)))
        .WillOnce(DoAll(SetArgPointee<2>(safestoreFLock), Return(0)));

    ASSERT_EQ(m_pluginCallback->m_serviceHealth, E_HEALTH_STATUS_BAD);
    result = m_pluginCallback->calculateHealth(m_mockSysCalls);
    logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos Safestore Process is now running"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos On Access Process is now running"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Sophos Threat Detector Process is now running"));
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Service Health has changed to: green"));
    ASSERT_EQ(result, E_HEALTH_STATUS_GOOD);
    //if this succeeds all process service healths are E_HEALTH_STATUS_GOOD
    ASSERT_EQ(m_pluginCallback->m_serviceHealth, E_HEALTH_STATUS_GOOD);
}
