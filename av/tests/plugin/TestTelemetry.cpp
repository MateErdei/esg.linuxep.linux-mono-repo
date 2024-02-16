// Copyright 2023 Sophos Limited. All rights reserved.

# define TEST_PUBLIC public

#include "pluginimpl/Telemetry.h"
#include "common/ApplicationPaths.h"
#include "common/StatusFile.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockSysCalls.h"
#include "PluginMemoryAppenderUsingTests.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = sophos_filesystem;

namespace
{
    class TestTelemetry : public PluginMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_basePath = fs::temp_directory_path();
            m_basePath /= test_info->test_case_name();
            m_basePath /= test_info->name();
            fs::remove_all(m_basePath);
            fs::create_directories(m_basePath);

            //creating file for vdl version
            m_vdlDirPath = m_basePath;
            m_vdlDirPath /= "chroot/susi/update_source/vdl";

            //For on-access-status tests
            auto var = m_basePath / "var";
            fs::create_directories(var);

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", m_basePath);

            m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            telemetry_ = std::make_unique<Plugin::Telemetry>(m_mockSysCalls, m_mockFileSystem.get());
        }

        void TearDown() override
        {
            fs::remove_all(m_basePath);
        }

        void setupInitialVersionFile()
        {
            m_versionFile = m_basePath;
            m_versionFile /= "VERSION.ini";

            std::ofstream versionFileStream;
            versionFileStream.open(m_versionFile);
            versionFileStream << "PRODUCT_NAME = SPL-Anti-Virus-Plugin" << std::endl;
            versionFileStream << "PRODUCT_VERSION = " << m_initialExpectedVersion << std::endl;
            versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
            versionFileStream << "COMMIT_HASH = " << m_initialExpectedCommitHash << std::endl;
            versionFileStream.close();
        }

        fs::path m_basePath;
        fs::path m_versionFile;
        fs::path m_vdlDirPath;
        std::string m_initialExpectedVersion = "1.2.3.456";
        std::string m_initialExpectedCommitHash = "dd9f7b10bb62c4b5f8eee1fede4bb4f4100a75c5";
        std::string m_initialExpectedPluginApiCommitHash = "93b8ec8736dcb5b4266f85b1b08110ebe19c7f03";

        std::unique_ptr<MockFileSystem> m_mockFileSystem;
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCalls;
        std::unique_ptr<Plugin::Telemetry> telemetry_;

        static Plugin::Telemetry realTelemetry()
        {
            auto realSyscalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
            auto* realFilesystem = Common::FileSystem::fileSystem();
            return Plugin::Telemetry{realSyscalls, realFilesystem};
        }

        static void createIdes(const unsigned long &count, const fs::path &dirPath, const char* contents="")
        {
            fs::create_directories(dirPath);

            for (unsigned long i=0; i<count; ++i)
            {
                std::stringstream ideFileName;
                ideFileName << "202306" << std::setfill('0') << std::setw(2) << i+1 << "01.ide";
                fs::path ideFilePath = dirPath / ideFileName.str();
                std::ofstream ideFs(ideFilePath);
                ideFs << contents;
                ideFs.close();

                std::stringstream vdbFileName;
                vdbFileName << "vdla" << std::setfill('0') << std::setw(2) << i+1 << ".vdb";
                fs::path vdbFilePath = dirPath / vdbFileName.str();
                std::ofstream vdbFs(vdbFilePath);
                vdbFs << contents;
                vdbFs.close();
            }
        }

    };
}

TEST_F(TestTelemetry, getProcessInfoReturnsZeroesWhenPidFileDoesNotExist)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Throw(
        Common::FileSystem::IFileSystemException("File does not exist.")));

    std::pair<long, long> expectedResult(0, 0);
    auto telemetry = std::make_unique<Plugin::Telemetry>(m_mockSysCalls, m_mockFileSystem.get());
    std::pair<long, long> result = telemetry->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Error accessing pid file: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsZeroesIfPidFileContentsAreMalformed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string badPidContents = "notanint";

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(badPidContents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    std::pair<long, long> expectedResult(0, 0);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Failed to read Pid file to int due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsZeroesIfPidDirectoryInProcIsMissing)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(pidProcDirectory)).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Process no longer running: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsZeroesIfAccessingPidDirectoryHasAFileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(pidProcDirectory)).WillOnce(Throw(
        Common::FileSystem::IFileSystemException("Cannot check this directory.")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Error accessing proc directory of pid: 1234 due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsZeroesIfStatFileSizeIsIncorrect)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::string statProcContents = "1 2 3";

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("The proc stat for 1234 is not of expected size"));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsMemoryCorrectAgeZeroWithInvalidSysInfoCall)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 100 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";
    std::pair<int, long> sysCallUptime(-1, 0);

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    EXPECT_CALL(*m_mockSysCalls, getSystemUpTime()).WillOnce(Return(sysCallUptime));

    long expectedMemoryValue = 500;
    long expectedAgeValue = 0;
    std::pair<long, long> expectedResult(expectedMemoryValue,expectedAgeValue);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Failed to retrieve system info, cant calculate process duration."));

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsCorrectValuesWhenSuccessful)
{
    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 450 23 500 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52";
    std::pair<int, long> sysCallUptime(0, 1000);

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    EXPECT_CALL(*m_mockSysCalls, getSystemUpTime()).WillOnce(Return(sysCallUptime));

    long expectedMemoryValue = 500;
    long expectedAgeValue = sysCallUptime.second - (450/sysconf(_SC_CLK_TCK));
    std::pair<long, long> expectedResult(expectedMemoryValue,expectedAgeValue);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsZeroesIfProcStatFileFailsToBeRead)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;
    std::optional<std::string> statProcContents = std::nullopt;

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Return(statProcContents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Stat file of Pid: 1234 is empty. Cannot report on Threat Detector Process to Telemetry: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getProcessInfoReturnsZeroesOnFileSystemExceptionWhenAccessingStatProcFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    log4cplus::Logger commonLogger = Common::Logging::getInstance("Common");
    commonLogger.addAppender(m_sharedAppender);

    Path threatDetectorPidFile = m_basePath / "chroot/var/threat_detector.pid";
    std::string pidFileContents = "1234";
    Path pidProcDirectory = "/proc/1234";
    int pidFileContentsConverted = 1234;

    EXPECT_CALL(*m_mockFileSystem, readFile(threatDetectorPidFile)).WillOnce(Return(pidFileContents));
    EXPECT_CALL(*m_mockFileSystem, isDirectory(pidProcDirectory)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readProcFile(pidFileContentsConverted, "stat")).WillOnce(Throw(
        Common::FileSystem::IFileSystemException("File does not exist.")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    std::pair<long, long> expectedResult(0,0);
    std::pair<long, long> result = telemetry_->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Error reading threat detector stat proc file due to: "));
    ASSERT_EQ(result, expectedResult);
}

TEST_F(TestTelemetry, getTelemetry_version)
{
    setupInitialVersionFile();
    std::string modifiedVersion = "1.2.3.457";

    auto realSyscalls = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
    auto* realFilesystem = Common::FileSystem::fileSystem();
    Plugin::Telemetry telemetry{realSyscalls, realFilesystem};

    auto initialTelemetry = json::parse(telemetry.getTelemetry());

    EXPECT_EQ(initialTelemetry["version"], m_initialExpectedVersion);

    std::ofstream versionFileStream;
    versionFileStream.open(m_versionFile);
    versionFileStream << "PRODUCT_NAME = SPL-Anti-Virus-Plugin" << std::endl;
    versionFileStream << "PRODUCT_VERSION = " << modifiedVersion << std::endl;
    versionFileStream << "BUILD_DATE = 1970-00-01" << std::endl;
    versionFileStream << "COMMIT_HASH = " << m_initialExpectedCommitHash << std::endl;
    versionFileStream.close();

    json modifiedTelemetry = json::parse(telemetry.getTelemetry());
    EXPECT_EQ(modifiedTelemetry["version"], modifiedVersion);
}

TEST_F(TestTelemetry, getTelemetry_vdlIdeCount)
{
    auto telemetry = realTelemetry();

    constexpr unsigned long initialExpectedVdlIdeCount = 3;
    createIdes(initialExpectedVdlIdeCount, m_vdlDirPath);

    json initialTelemetry = json::parse(telemetry.getTelemetry());
    EXPECT_EQ(initialTelemetry["vdl-ide-count"], initialExpectedVdlIdeCount);

    constexpr unsigned long modifiedExpectedVdlIdeCount = 4;
    createIdes(modifiedExpectedVdlIdeCount, m_vdlDirPath);
    json modifiedTelemetry = json::parse(telemetry.getTelemetry());

    EXPECT_EQ(modifiedTelemetry["vdl-ide-count"], modifiedExpectedVdlIdeCount);
}

TEST_F(TestTelemetry, getTelemetry_vdl_newest)
{
    auto telemetry = realTelemetry();

    constexpr unsigned long initialExpectedVdlIdeCount = 3;
    createIdes(initialExpectedVdlIdeCount, m_vdlDirPath);

    json initialTelemetry = json::parse(telemetry.getTelemetry());
    EXPECT_EQ(initialTelemetry["vdl-newest-ide"], "2023060301");

    constexpr unsigned long modifiedExpectedVdlIdeCount = 4;
    createIdes(modifiedExpectedVdlIdeCount, m_vdlDirPath);
    json modifiedTelemetry = json::parse(telemetry.getTelemetry());

    EXPECT_EQ(modifiedTelemetry["vdl-newest-ide"], "2023060401");
}

TEST_F(TestTelemetry, getTelemetry_vdl_size)
{
    auto telemetry = realTelemetry();

    constexpr unsigned long initialExpectedVdlIdeCount = 3;
    createIdes(initialExpectedVdlIdeCount, m_vdlDirPath);

    json initialTelemetry = json::parse(telemetry.getTelemetry());
    EXPECT_EQ(initialTelemetry["vdl-size"], 0);

    constexpr unsigned long modifiedExpectedVdlIdeCount = 3;
    createIdes(modifiedExpectedVdlIdeCount, m_vdlDirPath, "abcdefghij");
    json modifiedTelemetry = json::parse(telemetry.getTelemetry());
    EXPECT_EQ(modifiedTelemetry["vdl-size"], 60);
}

TEST_F(TestTelemetry, telemetry_reports_on_access_enabled)
{
    auto telemetry = realTelemetry();
    auto path = Plugin::getOnAccessStatusPath();
    auto statusFile = std::make_shared<common::StatusFile>(path);
    statusFile->enabled();
    auto modifiedTelemetry = json::parse(telemetry.getTelemetry());
    EXPECT_EQ(modifiedTelemetry["on-access-status"], true);
}

TEST_F(TestTelemetry, telemetry_reports_on_access_disabled)
{
    auto telemetry = realTelemetry();
    auto path = Plugin::getOnAccessStatusPath();
    auto statusFile = std::make_shared<common::StatusFile>(path);
    statusFile->disabled();
    auto modifiedTelemetry = json::parse(telemetry.getTelemetry());
    EXPECT_EQ(modifiedTelemetry["on-access-status"], false);
}