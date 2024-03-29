// Copyright 2020-2023 Sophos Limited. All rights reserved.

# define TEST_PUBLIC public

#include "PluginMemoryAppenderUsingTests.h"

#include "datatypes/AutoFd.h"
#include "pluginimpl/HealthStatus.h"
#include "pluginimpl/PluginCallback.h"
#include "pluginimpl/TaskQueue.h"
#include "sophos_on_access_process/fanotifyhandler/FanotifyHandler.h"
#include "unixsocket/threatDetectorSocket/ScanningServerConnectionThread.h"

#include "common/ApplicationPaths.h"
#include "common/FailedToInitializeSusiException.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockSysCalls.h"

#include "tests/common/ExpectedFanotifyInitFlags.h"
#include "tests/common/MockScanner.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace Plugin;
namespace fs = sophos_filesystem;
using json = nlohmann::json;

namespace
{
    class TestPluginCallback : public PluginMemoryAppenderUsingTests
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
            versionFileStream << "PRODUCT_NAME = SPL-Anti-Virus-Plugin" << std::endl;
            versionFileStream << "PRODUCT_VERSION = " << m_initialExpectedVersion << std::endl;
            versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
            versionFileStream << "COMMIT_HASH = " << m_initialExpectedCommitHash << std::endl;
            versionFileStream.close();

            // create var directories for PID files
            fs::create_directories(m_basePath / "var");
            fs::create_directories(m_basePath / "chroot/var");

            //creating initial ml library file
            fs::path libDirPath (m_basePath);
            libDirPath /= "chroot/susi/update_source/model";
            fs::create_directories(libDirPath);
            m_mlLibPath = libDirPath;
            m_mlLibPath /= "model.so";

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

            m_taskQueue = std::make_shared<Plugin::TaskQueue>();
            m_pluginCallback = std::make_shared<Plugin::PluginCallback>(m_taskQueue);
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
            m_sysCalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
        };

        void TearDown() override
        {
            fs::remove_all(m_basePath);
        }

        static void createIdes(const unsigned long &count, const fs::path &dirPath)
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

        static void createDataSetAFile(const std::string& path)
        {
            std::ofstream vdlVersionFileStream;
            vdlVersionFileStream.open(path);
            vdlVersionFileStream.close();
        }

        static void writeHexStringToFile(const std::string &hexString, const fs::path &filePath)
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

        void expectAbsentSusiStatusFile() const
        {
            auto path = Plugin::getThreatDetectorSusiUpdateStatusPath();
            EXPECT_CALL(*m_mockFileSystem, readFile(StrEq(path))).WillOnce(Throw(Common::FileSystem::IFileNotFoundException("FOOBAR")));
        }

        std::shared_ptr<Plugin::TaskQueue> m_taskQueue;
        std::shared_ptr<Plugin::PluginCallback> m_pluginCallback;
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCalls;
        std::shared_ptr<Common::SystemCallWrapper::SystemCallWrapper> m_sysCalls;
        std::unique_ptr<MockFileSystem> m_mockFileSystem;

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

TEST_F(TestPluginCallback, setTelemetryScanDetailsToZeroIfNoActivity)
{
    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(modifiedTelemetry["scan-now-count"], 0);
    EXPECT_EQ(modifiedTelemetry["scheduled-scan-count"], 0);
    EXPECT_EQ(modifiedTelemetry["on-access-threat-count"], 0);
    EXPECT_EQ(modifiedTelemetry["on-access-threat-eicar-count"], 0);
    EXPECT_EQ(modifiedTelemetry["on-demand-threat-count"], 0);
    EXPECT_EQ(modifiedTelemetry["on-demand-threat-eicar-count"], 0);
}

TEST_F(TestPluginCallback, getTelemetry_threatHealth_fileDoesNotExist)
{
    json telemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(telemetry["threatHealth"], E_THREAT_HEALTH_STATUS_GOOD);
}

TEST_F(TestPluginCallback, getHealthReturnsBadWhenPidfileExistsButIsNotLocked)
{
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::ofstream(threatDetectorPidFile).close();
    Path soapdPidFile = m_basePath / "var/soapd.pid";
    std::ofstream(soapdPidFile).close();

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_sysCalls);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsBadWhenDormantFlagExists)
{
    int fileDescriptor = 123;
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    EXPECT_CALL(*m_mockSysCalls, flock(fileDescriptor, LOCK_EX | LOCK_NB)).WillRepeatedly(SetErrnoAndReturn(EWOULDBLOCK, -1));

    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    
    long expectedResult = E_HEALTH_STATUS_BAD;
    m_pluginCallback->calculateSafeStoreHealthStatus(m_sysCalls);

    ASSERT_EQ(m_pluginCallback->m_safestoreServiceStatus, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsBadWhenPidFileDoesNotExistAndShutdownFileHasExpired)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    
    EXPECT_CALL(*m_mockFileSystem, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, lastModifiedTime(shutdownFilePath)).WillOnce(Return(std::time(nullptr) - 60));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    
    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_sysCalls);

    std::stringstream errMsg;
    errMsg << "Unable to open PID file " << threatDetectorPidFile << " (No such file or directory), assume process not running";
    EXPECT_TRUE(appenderContains(errMsg.str()));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, getHealthReturnsBadWhenPidFileDoesNotExistAndShutdownHasAFileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    EXPECT_CALL(*m_mockFileSystem, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, lastModifiedTime(shutdownFilePath)).WillOnce(Throw(
            Common::FileSystem::IFileSystemException("Shutdown file read error.")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    
    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_sysCalls);

    EXPECT_TRUE(appenderContains("Error accessing threat detector expected shutdown file: "));
    std::stringstream errMsg;
    errMsg << "Unable to open PID file " << threatDetectorPidFile << " (No such file or directory), assume process not running";
    EXPECT_TRUE(appenderContains(errMsg.str()));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsGoodIfLockCannotBeTakenOnPidFilesAndStatusHealthy)
{
    int fileDescriptor = 123;
    expectAbsentSusiStatusFile();
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    EXPECT_CALL(*m_mockSysCalls, flock(fileDescriptor, LOCK_EX | LOCK_NB)).WillRepeatedly(SetErrnoAndReturn(EWOULDBLOCK, -1));

    long expectedResult = E_HEALTH_STATUS_GOOD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsBadIfLockCannotBeTakenOnPidFilesButStatusUnhealthy)
{
    int fileDescriptor = 123;
    expectAbsentSusiStatusFile();
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    EXPECT_CALL(*m_mockSysCalls, flock(fileDescriptor, LOCK_EX | LOCK_NB)).WillRepeatedly(SetErrnoAndReturn(EWOULDBLOCK, -1));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsBadIfLockCanBeTakenOnThreatDetectorPidFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    EXPECT_CALL(*m_mockFileSystem, exists(shutdownFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, lastModifiedTime(shutdownFilePath)).WillOnce(Throw(
        Common::FileSystem::IFileSystemException("Shutdown file read error.")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    
    int other_fd = 123;
    int threat_detector_fd = 321;

    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(other_fd));
    EXPECT_CALL(*m_mockSysCalls, _open(::testing::StrEq(threatDetectorPidFile), O_RDONLY, 0644)).WillOnce(Return(threat_detector_fd));
    EXPECT_CALL(*m_mockSysCalls, flock(other_fd, LOCK_EX | LOCK_NB)).WillRepeatedly(SetErrnoAndReturn(EWOULDBLOCK, -1));
    EXPECT_CALL(*m_mockSysCalls, flock(threat_detector_fd, LOCK_EX | LOCK_NB)).WillRepeatedly(Return(0));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Lock acquired on PID file "));
    EXPECT_TRUE(appenderContains(" assume process not running"));
    EXPECT_TRUE(appenderContains("Sophos Threat Detector Process is not running, turning service health to red"));
    EXPECT_TRUE(appenderContains("Service Health has changed to: red"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsBadIfLockCanBeTakenOnSoapdPidFileRegardlessOfOnaccessUnhealthyFlag)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path soapdPidFile = m_basePath / "var/soapd.pid";

    int other_fd = 123;
    int soapd_fd = 321;

    expectAbsentSusiStatusFile();
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(other_fd));
    EXPECT_CALL(*m_mockSysCalls, _open(::testing::StrEq(soapdPidFile), O_RDONLY, 0644)).WillOnce(Return(soapd_fd));
    EXPECT_CALL(*m_mockSysCalls, flock(other_fd, LOCK_EX | LOCK_NB)).WillRepeatedly(SetErrnoAndReturn(EWOULDBLOCK, -1));
    EXPECT_CALL(*m_mockSysCalls, flock(soapd_fd, LOCK_EX | LOCK_NB)).WillRepeatedly(Return(0));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Lock acquired on PID file "));
    EXPECT_TRUE(appenderContains(" assume process not running"));
    EXPECT_TRUE(appenderContains("Sophos On Access Process is not running, turning service health to red"));
    EXPECT_TRUE(appenderContains("Service Health has changed to: red"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestPluginCallback, calculateHealthReturnsBadIfLockCanBeTakenOnSafeStorePidFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path safestorePidFile = Plugin::getSafeStorePidPath();

    int other_fd = 123;
    int safestore_fd = 321;

    expectAbsentSusiStatusFile();
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(other_fd));
    EXPECT_CALL(*m_mockSysCalls, _open(::testing::StrEq(safestorePidFile), O_RDONLY, 0644)).WillOnce(Return(safestore_fd));
    EXPECT_CALL(*m_mockSysCalls, flock(other_fd, LOCK_EX | LOCK_NB)).WillRepeatedly(SetErrnoAndReturn(EWOULDBLOCK, -1));
    EXPECT_CALL(*m_mockSysCalls, flock(safestore_fd, LOCK_EX | LOCK_NB)).WillRepeatedly(Return(0));

    long expectedResult = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Lock acquired on PID file "));
    EXPECT_TRUE(appenderContains(" assume process not running"));
    EXPECT_TRUE(appenderContains("Sophos SafeStore Process is not running, turning service health to red"));
    EXPECT_TRUE(appenderContains("Service Health has changed to: red"));
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

    EXPECT_CALL(*m_mockFileSystem, exists(shutdownFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillRepeatedly(Return(threatDetectorPidFileContents));
    EXPECT_CALL(*m_mockFileSystem, readFile(soapdPidFile)).WillRepeatedly(Return(soapdPidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(threatDetectorPidProcDirectory)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(soapdPidProcDirectory)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readProcFile(threatDetectorPidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    
    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());

    long expectAgeValueToBeGreaterThan = 0;
    long expectedMemoryValue = 500;

    ASSERT_EQ(modifiedTelemetry["threatMemoryUsage"], expectedMemoryValue);
    ASSERT_NE(modifiedTelemetry["threatProcessAge"], expectAgeValueToBeGreaterThan);
}

TEST_F(TestPluginCallback, checkCalculateServiceHealthLogsTheRightThings)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path shutdownFilePath = m_basePath / "chroot/var/threat_detector_expected_shutdown";

    expectAbsentSusiStatusFile();
    EXPECT_CALL(*m_mockFileSystem, exists(shutdownFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getOnAccessUnhealthyFlagPath())).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getThreatDetectorUnhealthyFlagPath())).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, lastModifiedTime(shutdownFilePath)).WillRepeatedly(Throw(
        Common::FileSystem::IFileSystemException("Shutdown file read error.")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    int fileDescriptor = 123;
    EXPECT_CALL(*m_mockSysCalls, _open(_, O_RDONLY, 0644)).WillRepeatedly(Return(fileDescriptor));
    EXPECT_CALL(*m_mockSysCalls, flock(fileDescriptor, LOCK_EX | LOCK_NB)).WillRepeatedly(Return(0));

    m_pluginCallback->m_safestoreServiceStatus = E_HEALTH_STATUS_BAD;
    m_pluginCallback->m_soapServiceStatus = E_HEALTH_STATUS_BAD;
    m_pluginCallback->m_threatDetectorServiceStatus = E_HEALTH_STATUS_BAD;
    m_pluginCallback->m_serviceHealth = E_HEALTH_STATUS_BAD;
    long result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Lock acquired on PID file"));
    EXPECT_TRUE(appenderContains("assume process not running"));
    EXPECT_FALSE(appenderContains("Sophos SafeStore Process is not running, turning service health to red"));
    EXPECT_FALSE(appenderContains("Sophos On Access Process is not running, turning service health to red"));
    EXPECT_FALSE(appenderContains("Sophos Threat Detector Process is not running, turning service health to red"));
    EXPECT_FALSE(appenderContains("Service Health has changed to"));
    ASSERT_EQ(result, E_HEALTH_STATUS_BAD);

    EXPECT_CALL(*m_mockSysCalls, flock(fileDescriptor, LOCK_EX | LOCK_NB)).WillRepeatedly(SetErrnoAndReturn(EWOULDBLOCK, -1));

    ASSERT_EQ(m_pluginCallback->m_serviceHealth, E_HEALTH_STATUS_BAD);
    result = m_pluginCallback->calculateHealth(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Sophos SafeStore Process is now healthy"));
    EXPECT_TRUE(appenderContains("Sophos On Access Process is now healthy"));
    EXPECT_TRUE(appenderContains("Sophos Threat Detector Process is now healthy"));
    EXPECT_TRUE(appenderContains("Service Health has changed to: green"));
    ASSERT_EQ(result, E_HEALTH_STATUS_GOOD);
    //if this succeeds all process service healths are E_HEALTH_STATUS_GOOD
    ASSERT_EQ(m_pluginCallback->m_serviceHealth, E_HEALTH_STATUS_GOOD);
}

TEST_F(TestPluginCallback, calculateSoapHealthStatusDetectsExistenceOfTheUnhealthyFlagFileCreatedByFanotifyHandler)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFd = -1;
    EXPECT_CALL(*m_mockSysCalls, fanotify_init(EXPECTED_FANOTIFY_FLAGS,
                                               O_RDONLY | O_CLOEXEC | O_LARGEFILE)).WillOnce(Return(fanotifyFd));

    sophos_on_access_process::fanotifyhandler::FanotifyHandler handler(m_mockSysCalls);
    EXPECT_THROW(handler.init(), std::runtime_error);

    m_pluginCallback->calculateSoapHealthStatus(m_sysCalls);
    ASSERT_EQ(m_pluginCallback->m_soapServiceStatus, E_HEALTH_STATUS_BAD);
    EXPECT_TRUE(appenderContains("Sophos On Access Process is unhealthy, turning service health to red"));
}

TEST_F(TestPluginCallback, calculateThreatDetectorStatusDetectsUnhealthyFlagFileCreatedByScanningServerConnectionThread)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto mockScannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();
    EXPECT_CALL(*mockScannerFactory, createScanner(_, _, _)).WillOnce(Throw(FailedToInitializeSusiException("failedtoinitialise")));

    auto request = std::make_shared<scan_messages::ScanRequest>();
    std::string errMsg;
    scan_messages::ScanResponse result;
    datatypes::AutoFd fd(::open("/dev/null", O_RDONLY));

    unixsocket::ScanningServerConnectionThread scanningThread(fd, mockScannerFactory, m_mockSysCalls, 1);
    EXPECT_FALSE(scanningThread.attemptScan(request, errMsg, result, fd));

    m_pluginCallback->calculateThreatDetectorHealthStatus(m_sysCalls);
    ASSERT_EQ(m_pluginCallback->m_threatDetectorServiceStatus, E_HEALTH_STATUS_BAD);
    EXPECT_TRUE(appenderContains("Sophos Threat Detector Process is unhealthy, turning service health to red"));
}

TEST_F(TestPluginCallback, calculateThreatDetectorStatus_bad_susi_update)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto mockScannerFactory = std::make_shared<StrictMock<MockScannerFactory>>();

    // SUSI update status - returns red unhealthy
    {
        auto path = Plugin::getThreatDetectorSusiUpdateStatusPath();
        EXPECT_CALL(*m_mockFileSystem, readFile(StrEq(path))).WillOnce(Return(R"({
        "success":false
    })"));
    }
    // Unhealthy file check - returns green healthy
    {
        auto path = Plugin::getThreatDetectorUnhealthyFlagPath();
        EXPECT_CALL(*m_mockFileSystem, isFile(StrEq(path))).WillOnce(Return(false));
    }

    // PID check - returns green healthy
    EXPECT_CALL(*m_mockSysCalls, _open(_,_,_)).WillOnce(Return(42));
    EXPECT_CALL(*m_mockSysCalls, flock(42, _)).WillOnce(SetErrnoAndReturn(EAGAIN, -1));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};
    auto ret = m_pluginCallback->calculateThreatDetectorHealthStatus(m_mockSysCalls);
    EXPECT_EQ(ret, E_HEALTH_STATUS_BAD);
    EXPECT_EQ(m_pluginCallback->m_threatDetectorServiceStatus, E_HEALTH_STATUS_BAD);
    EXPECT_TRUE(appenderContains("SUSI update failed, turning service health to red"));
}

TEST_F(TestPluginCallback, sendStatus)
{
    m_pluginCallback->sendStatus();
    ASSERT_FALSE(m_taskQueue->empty());
    auto task = m_taskQueue->pop();
    ASSERT_EQ(task.taskType, Plugin::Task::TaskType::SendStatus);
    EXPECT_THAT(task.Content, HasSubstr("<on-access>false</on-access>"));
    EXPECT_THAT(task.Content, HasSubstr(R"(<csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="NoRef" RevID="" policyType="2"/>)"));
}

TEST_F(TestPluginCallback, sendStatusWithRef)
{
    m_pluginCallback->sendStatus("MyReference");
    ASSERT_FALSE(m_taskQueue->empty());
    auto task = m_taskQueue->pop();
    ASSERT_EQ(task.taskType, Plugin::Task::TaskType::SendStatus);
    EXPECT_THAT(task.Content, HasSubstr(R"(<csc:CompRes xmlns:csc="com.sophos\msys\csc" Res="Same" RevID="MyReference" policyType="2"/>)"));
}

TEST_F(TestPluginCallback, sendStatusOnAccess)
{
    m_pluginCallback->setOnAccessEnabled(true);
    m_pluginCallback->sendStatus();
    ASSERT_FALSE(m_taskQueue->empty());
    auto task = m_taskQueue->pop();
    ASSERT_EQ(task.taskType, Plugin::Task::TaskType::SendStatus);
    EXPECT_THAT(task.Content, HasSubstr("<on-access>true</on-access>"));
}
