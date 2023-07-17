// Copyright 2023 Sophos All rights reserved.

#include "pluginimpl/Telemetry.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockSysCalls.h"
#include "PluginMemoryAppenderUsingTests.h"

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

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", m_basePath);

            m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
            m_mockSysCalls = std::make_shared<StrictMock<MockSystemCallWrapper>>();
            m_pluginCallback = std::make_unique<Plugin::Telemetry>();
        }

        void TearDown() override
        {
            fs::remove_all(m_basePath);
        }

        fs::path m_basePath;
        std::unique_ptr<MockFileSystem> m_mockFileSystem;
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCalls;
        std::unique_ptr<Plugin::Telemetry> m_pluginCallback;
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
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(m_mockFileSystem)};

    std::pair<long, long> expectedResult(0, 0);
    auto telemetry = std::make_unique<Plugin::Telemetry>();
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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

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
    std::pair<long, long> result = m_pluginCallback->getThreatScannerProcessinfo(m_mockSysCalls);

    EXPECT_TRUE(appenderContains("Error reading threat detector stat proc file due to: "));
    ASSERT_EQ(result, expectedResult);
}
