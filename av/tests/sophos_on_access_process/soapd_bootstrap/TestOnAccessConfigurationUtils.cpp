// Copyright 2022 Sophos Limited. All rights reserved.

#include "sophos_on_access_process/soapd_bootstrap/OnAccessConfigurationUtils.h"

#include "Common/FileSystem/IFileSystemException.h"

#include "../SoapMemoryAppenderUsingTests.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include "tests/common/LogInitializedTests.h"
#include "tests/datatypes/MockSysCalls.h"

#include <gtest/gtest.h>

#include <tuple>

using namespace sophos_on_access_process::OnAccessConfig;
namespace fs = sophos_filesystem;

namespace
{
    class TestOnAccessConfigurationUtils : public SoapMemoryAppenderUsingTests
    {
    protected:
        fs::path m_testDir;
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
            appConfig.setData("PLUGIN_INSTALL", m_testDir );

            m_oaPolicyConfigPath = m_testDir / "var/on_access_policy.json";
            m_oaLocalSettingsPath = m_testDir / "var/on_access_local_settings.json";
            m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
            m_defaultTestExclusions.emplace_back("/mnt/");
            m_defaultTestExclusions.emplace_back("/uk-filer5/");

            m_sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
            m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
        }

        void expectReadConfig(MockFileSystem& mock, const std::string& contents)
        {
            EXPECT_CALL(mock, readFile(m_oaLocalSettingsPath)).WillOnce(Return(contents));
        }

        std::string m_oaPolicyConfigPath;
        std::string m_oaLocalSettingsPath;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
        std::vector<common::Exclusion> m_defaultTestExclusions;
        std::shared_ptr<datatypes::SystemCallWrapper> m_sysCallWrapper;
        std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
    };
}

// Policy config ==================================================================

TEST_F(TestOnAccessConfigurationUtils, readPolicyConfigFile)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaPolicyConfigPath)).WillOnce(Return("x"));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    ASSERT_EQ(readPolicyConfigFile(), "x");
}

TEST_F(TestOnAccessConfigurationUtils, readConfigFileReadThrows)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaPolicyConfigPath)).WillOnce(
        Throw(Common::FileSystem::IFileSystemException("bang")));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    ASSERT_EQ(readPolicyConfigFile(), "");
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson)
{
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions = m_defaultTestExclusions;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString), expectedResult);

    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = m_defaultTestExclusions;

    ASSERT_NE(parseOnAccessPolicySettingsFromJson(jsonString), expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson_missingFields)
{
    std::string jsonString = R"({"excludeRemoteFiles":"true"})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString), expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessSettingsFromJsonInvalidJson)
{
    std::string jsonString = R"({"enabled":"I think","excludeRemoteFiles":"therefore","exclusions":["I","am"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions.emplace_back("I");
    expectedResult.exclusions.emplace_back("am");

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString), expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessSettingsFromJsonInvalidJsonSyntax)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"(this is going to break)";

    OnAccessConfiguration expectedResult {};

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString), expectedResult);
    EXPECT_TRUE(appenderContains("Failed to parse json configuration, keeping existing settings"));
}

// Flags config ===============================================================

TEST_F(TestOnAccessConfigurationUtils, parseFlagConfiguration)
{
    std::string jsonString = R"({"oa_enabled":false})";
    EXPECT_FALSE(parseFlagConfiguration(jsonString));

    jsonString = R"({"oa_enabled":true})";
    EXPECT_TRUE(parseFlagConfiguration(jsonString));
}

TEST_F(TestOnAccessConfigurationUtils, parseFlagConfigurationFromJsonInvalidJsonSyntax)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"(this is going to break)";

    EXPECT_FALSE(parseFlagConfiguration(jsonString));
    EXPECT_TRUE(appenderContains("Failed to parse flag configuration, keeping existing settings"));
}

// Local Settings ================================================================

TEST_F(TestOnAccessConfigurationUtils, readSettingsFromEmptyFile)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(1));
    expectReadConfig(*filesystemMock, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, sophos_on_access_process::OnAccessConfig::defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, sophos_on_access_process::OnAccessConfig::defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, sophos_on_access_process::OnAccessConfig::defaultCacheAllEvents);
    EXPECT_EQ(result.numScanThreads, 1); // 1 CPU core
}

TEST_F(TestOnAccessConfigurationUtils, zeroCpu)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(0));
    expectReadConfig(*filesystemMock, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.numScanThreads, sophos_on_access_process::OnAccessConfig::defaultScanningThreads);
}

TEST_F(TestOnAccessConfigurationUtils, tenCPUCores)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 5);
}

TEST_F(TestOnAccessConfigurationUtils, emptyJSON)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, "{}");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, sophos_on_access_process::OnAccessConfig::defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, sophos_on_access_process::OnAccessConfig::defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, sophos_on_access_process::OnAccessConfig::defaultCacheAllEvents);
    EXPECT_EQ(result.numScanThreads, 5);
}

TEST_F(TestOnAccessConfigurationUtils, jsonOverrides)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, R"({
        "dumpPerfData" : true,
        "cacheAllEvents" : true,
        "maxscanqueuesize" : 1038,
        "numThreads" : 42
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, 1038);
    EXPECT_EQ(result.dumpPerfData, true);
    EXPECT_EQ(result.cacheAllEvents, true);
    EXPECT_EQ(result.numScanThreads, 42);
}

TEST_F(TestOnAccessConfigurationUtils, minLimitQueue)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, R"({
        "maxscanqueuesize" : 38
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, 1000);
}

TEST_F(TestOnAccessConfigurationUtils, maxThreadCount)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, R"({
        "numThreads" : 999999
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 100);
}

TEST_F(TestOnAccessConfigurationUtils, hardwareConcurrencyBypassesMaxThreads)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(400));
    expectReadConfig(*filesystemMock, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 200);
}

namespace
{
    void readLocalSettingsFile(size_t& maxScanQueueSize, int& numScanThreads, bool& dumpPerfData, const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls)
    {
        auto settings = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCalls);
        maxScanQueueSize = settings.maxScanQueueSize;
        numScanThreads = settings.numScanThreads;
        dumpPerfData = settings.dumpPerfData;
    }
}

TEST_F(TestOnAccessConfigurationUtils, parseProductConfigEmptyKeepsDefaults)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = true;

    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_sysCallWrapper);

    auto numScanThreads = (std::thread::hardware_concurrency() + 1) / 2;
    EXPECT_EQ(maxScanQueueItems, defaultMaxScanQueueSize);
    EXPECT_EQ(numThreads, numScanThreads);
    EXPECT_FALSE(dumpPerfData);

    std::stringstream logmsg;
    logmsg << "Setting from defaults: Max queue size set to " << defaultMaxScanQueueSize << " and Max threads set to " << numScanThreads;
    EXPECT_TRUE(appenderContains(logmsg.str()));
}

class TestOnAccessConfigUtilsParameterized
    : public ::testing::TestWithParam<std::pair<int, int>>
{
protected:
    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, "" );
        appConfig.setData("PLUGIN_INSTALL", "" );

        m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
        m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        m_loggingSetup = Common::Logging::LOGOFFFORTEST();
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
};

INSTANTIATE_TEST_SUITE_P(
    TestOnAccessConfigUtils,
    TestOnAccessConfigUtilsParameterized,
    ::testing::Values(
        std::make_pair(0, defaultScanningThreads),
        std::make_pair(1, 1),
        std::make_pair(2, 1),
        std::make_pair(3, 2),
        std::make_pair(8, 4),
        std::make_pair(16, 8)
            ));

TEST_P(TestOnAccessConfigUtilsParameterized, parseProductConfigEmpty_numberOfCoresDeterminesNumberOfScanningThreads)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = true;

    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(std::get<0>(GetParam())));
    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_mockSysCallWrapper);

    auto numScanThreads = std::get<1>(GetParam());
    EXPECT_EQ(maxScanQueueItems, defaultMaxScanQueueSize);
    EXPECT_EQ(numThreads, numScanThreads);
    EXPECT_FALSE(dumpPerfData);
}

TEST_F(TestOnAccessConfigurationUtils, parseProductConfigIgnoresBadvalues)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(R"({"numThreads": "ha", "maxscanqueuesize": "ha", "dumpPerfData": "ha"})"));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = true;

    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_sysCallWrapper);

    auto numScanThreads = (std::thread::hardware_concurrency() + 1) / 2;
    EXPECT_EQ(maxScanQueueItems, defaultMaxScanQueueSize);
    EXPECT_EQ(numThreads, numScanThreads);
    EXPECT_FALSE(dumpPerfData);

    EXPECT_TRUE(appenderContains("Failed to read product config file info: [json.exception.type_error.302] type must be number, but is string"));
    std::stringstream logmsg;
    logmsg << "Setting from defaults: Max queue size set to " << defaultMaxScanQueueSize << " and Max threads set to " << numScanThreads;
    EXPECT_TRUE(appenderContains(logmsg.str()));
}

TEST_F(TestOnAccessConfigurationUtils, parseProductConfigSetsDefaultWhenFileDoenstExist)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = true;

    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_sysCallWrapper);

    auto numScanThreads = (std::thread::hardware_concurrency() + 1) / 2;
    EXPECT_EQ(maxScanQueueItems, defaultMaxScanQueueSize);
    EXPECT_EQ(numThreads, numScanThreads);
    EXPECT_FALSE(dumpPerfData);

    std::stringstream logmsg;
    logmsg << "Setting from defaults: Max queue size set to " << defaultMaxScanQueueSize << " and Max threads set to " << numScanThreads;
    EXPECT_TRUE(appenderContains(logmsg.str()));
}


TEST_F(TestOnAccessConfigurationUtils, parseProductConfigSetsToProvidedValuesWhenFileExists)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(R"({"numThreads": 20, "maxscanqueuesize": 2000, "dumpPerfData": true})"));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = false;

    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_sysCallWrapper);

    EXPECT_EQ(maxScanQueueItems, 2000);
    EXPECT_EQ(numThreads, 20);
    EXPECT_TRUE(dumpPerfData);

    EXPECT_TRUE(appenderContains("Setting from file: Max queue size set to 2000 and Max threads set to 20"));
}

TEST_F(TestOnAccessConfigurationUtils, parseProductConfig_missingFields)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(R"({"numThreads": 20, "maxscanqueuesize": 2000})"));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = false;

    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_sysCallWrapper);

    EXPECT_EQ(maxScanQueueItems, 2000);
    EXPECT_EQ(numThreads, 20);
    EXPECT_FALSE(dumpPerfData);

    EXPECT_TRUE(appenderContains("Setting from file: Max queue size set to 2000 and Max threads set to 20"));
}

TEST_F(TestOnAccessConfigurationUtils, parseProductConfigSetsToMaxPossibleValueWhenProvidedValuesToHigh)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::stringstream returnStr;
    returnStr << R"({"numThreads": )" << (maxAllowedScanningThreads + 1) << R"(, "maxscanqueuesize": )" << (maxAllowedQueueSize + 1) << R"(, "dumpPerfData": true})";
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(returnStr.str()));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = false;

    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_sysCallWrapper);

    EXPECT_EQ(maxScanQueueItems, maxAllowedQueueSize);
    EXPECT_EQ(numThreads, maxAllowedScanningThreads);
    EXPECT_TRUE(dumpPerfData);

    std::stringstream queueSizeLogMsg;
    queueSizeLogMsg << "Queue size of " << (maxAllowedQueueSize + 1) << " is greater than maximum allowed of " << maxAllowedQueueSize;

    std::stringstream threadCountLogMsg;
    threadCountLogMsg << "Scanning Thread count of " << (maxAllowedScanningThreads + 1) << " is greater than maximum allowed of " << maxAllowedScanningThreads;

    std::stringstream settingLogMsg;
    settingLogMsg << "Setting from file: Max queue size set to " << maxAllowedQueueSize << " and Max threads set to " << maxAllowedScanningThreads;

    EXPECT_TRUE(waitForLog(queueSizeLogMsg.str()));
    EXPECT_TRUE(waitForLog(threadCountLogMsg.str()));
    EXPECT_TRUE(waitForLog(settingLogMsg.str()));
}


TEST_F(TestOnAccessConfigurationUtils, parseProductConfigSetsToMinPossibleValueWhenProvidedValuesToLow)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::stringstream returnStr;
    returnStr << R"({"numThreads": )" << (minAllowedScanningThreads - 1) << R"(, "maxscanqueuesize": )" << (minAllowedQueueSize - 1) << R"(, "dumpPerfData": true})";
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(returnStr.str()));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = false;

    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_sysCallWrapper);

    EXPECT_EQ(maxScanQueueItems, minAllowedQueueSize);
    EXPECT_EQ(numThreads, minAllowedScanningThreads);
    EXPECT_TRUE(dumpPerfData);

    std::stringstream queueSizeLogMsg;
    queueSizeLogMsg << "Queue size of " << (minAllowedQueueSize - 1) << " is less than minimum allowed of " << minAllowedQueueSize;

    std::stringstream threadCountLogMsg;
    threadCountLogMsg << "Scanning Thread count of " << (minAllowedScanningThreads - 1) << " is less than minimum allowed of " << minAllowedScanningThreads;

    std::stringstream settingLogMsg;
    settingLogMsg << "Setting from file: Max queue size set to " << minAllowedQueueSize << " and Max threads set to " << minAllowedScanningThreads;

    EXPECT_TRUE(waitForLog(queueSizeLogMsg.str()));
    EXPECT_TRUE(waitForLog(threadCountLogMsg.str()));
    EXPECT_TRUE(waitForLog(settingLogMsg.str()));
}