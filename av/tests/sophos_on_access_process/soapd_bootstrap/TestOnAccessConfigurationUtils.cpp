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
            m_oaFlagsPath = m_testDir / "var/oa_flag.json";
            m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
            m_defaultTestExclusions.emplace_back("/mnt/");
            m_defaultTestExclusions.emplace_back("/uk-filer5/");
            m_localSettingsNotUsedMessage = "Some or all local settings weren't set from file: Queue Size: 100000, Max threads: 5, Perf dump: false, Cache all events: false, Uncache detections: true";

            m_sysCallWrapper = std::make_shared<datatypes::SystemCallWrapper>();
            m_mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
        }

        void expectReadConfig(MockFileSystem& mock, const std::string& contents)
        {
            EXPECT_CALL(mock, readFile(m_oaLocalSettingsPath)).WillOnce(Return(contents));
        }

        OnAccessConfiguration m_testConfig{};
        std::string m_oaPolicyConfigPath;
        std::string m_oaLocalSettingsPath;
        std::string m_oaFlagsPath;
        std::string m_localSettingsNotUsedMessage;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
        std::vector<common::Exclusion> m_defaultTestExclusions;
        std::shared_ptr<datatypes::SystemCallWrapper> m_sysCallWrapper;
        std::shared_ptr<NiceMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
        //hardware_concurrency syscall default return value will result in defaultThreads below
        const int m_defaultThreads = 5;
    };

    void readLocalSettingsFile(size_t& maxScanQueueSize, int& numScanThreads, bool& dumpPerfData, const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls)
    {
        auto settings = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCalls);
        maxScanQueueSize = settings.maxScanQueueSize;
        numScanThreads = settings.numScanThreads;
        dumpPerfData = settings.dumpPerfData;
    }
}
// Policy config ==================================================================

TEST_F(TestOnAccessConfigurationUtils, emptyJSONPolicy)
{
    std::string jsonString = "{}";

    OnAccessConfiguration expectedResult{};

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), false);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, readPolicyConfigFile)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaPolicyConfigPath)).WillOnce(Return("x"));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    EXPECT_EQ(readPolicyConfigFile(), "x");
}

TEST_F(TestOnAccessConfigurationUtils, readConfigFileReadThrows)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaPolicyConfigPath)).WillOnce(
        Throw(Common::FileSystem::IFileSystemException("Error, Failed to read file: pretend/file.txt, file does not exist")));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    EXPECT_EQ(readPolicyConfigFile(), "");
    EXPECT_TRUE(appenderContains("Failed to read on-access configuration, keeping existing configuration"));
}


TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson)
{
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions = m_defaultTestExclusions;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);

    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = m_defaultTestExclusions;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_NE(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJsonDuplicates)
{
    std::string jsonString = R"({
        "enabled":"true",
        "excludeRemoteFiles":"false",
        "enabled":"false",
        "excludeRemoteFiles":"true",
        "exclusions":["/tmp/","/uk-filer6/"],
        "exclusions":["/mnt/","/uk-filer5/"]
        })";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = m_defaultTestExclusions;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson_missingExcludeRemote)
{
    std::string jsonString = R"({"enabled":"true","exclusions":["/mnt/","/uk-filer5/"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions = m_defaultTestExclusions;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson_missingEnabled)
{
    std::string jsonString = R"({"excludeRemoteFiles":"true","exclusions":["/mnt/","/uk-filer5/"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = m_defaultTestExclusions;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson_missingExclusions)
{
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"true"})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = true;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessSettingsFromJsonInvalidJson)
{
    std::string jsonString = R"({"enabled":"I think","excludeRemoteFiles":"therefore","exclusions":["I","am"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions.emplace_back("I");
    expectedResult.exclusions.emplace_back("am");

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, enablesOnAccessWithNumberInField)
{
    std::string jsonString = R"({"enabled":123,"excludeRemoteFiles":"false","exclusions":["exc","lude"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions.emplace_back("exc");
    expectedResult.exclusions.emplace_back("lude");

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, excludesRemoteFilesWithNumberInField)
{
    std::string jsonString = R"({"enabled":"false","excludeRemoteFiles":123,"exclusions":["exc","lude"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions.emplace_back("exc");
    expectedResult.exclusions.emplace_back("lude");

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

/*TEST_F(TestOnAccessConfigurationUtils, parseOnAccessSettingsFromJsonKeysWithInvalidValueTypes)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"enabled":24311ddaf53,"excludeRemoteFiles":"true","exclusions":["exc","lude"]})";

    OnAccessConfiguration expectedResult;

    EXPECT_EQ(parseOnAccessPolicySettingsFromJson(jsonString), expectedResult);

    EXPECT_TRUE(appenderContains("Failed to parse json configuration due to parse error, reason: [json.exception.parse_error.101] parse error at line 1, column 17: syntax error while parsing object - invalid literal; last read: '24311d'; expected '}"));
    EXPECT_TRUE(appenderContains("Failed to parse json configuration, keeping existing settings"));

}*/

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessSettingsFromJsonInvalidJsonSyntax)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"(this is going to break)";

    OnAccessConfiguration expectedResult {};

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), false);
    EXPECT_EQ(m_testConfig, expectedResult);
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

TEST_F(TestOnAccessConfigurationUtils, parseFlagConfigurationDuplicates)
{
    std::string jsonString = R"({"oa_enabled":true,"oa_enabled":false})";
    EXPECT_FALSE(parseFlagConfiguration(jsonString));

    jsonString = R"({"oa_enabled":false,"oa_enabled":true})";
    EXPECT_TRUE(parseFlagConfiguration(jsonString));
}

TEST_F(TestOnAccessConfigurationUtils, parseFlagConfigurationFromJsonInvalidJsonSyntax)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"(this is going to break)";

    EXPECT_FALSE(parseFlagConfiguration(jsonString));

    EXPECT_TRUE(appenderContains("Failed to parse json configuration of flags due to parse error, reason: "));
    EXPECT_TRUE(appenderContains("Failed to parse flag configuration, keeping existing settings"));
}

TEST_F(TestOnAccessConfigurationUtils, parseFlagConfigurationThrows)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaFlagsPath)).WillOnce(
        Throw(Common::FileSystem::IFileSystemException("Error, Failed to read file: pretend/file.txt, file does not exist")));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    EXPECT_EQ(readFlagConfigFile(), "");
    EXPECT_TRUE(appenderContains("Failed to read flag configuration, keeping existing configuration"));
}

TEST_F(TestOnAccessConfigurationUtils, parseFlagConfigurationFromEmptyJson)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = "{}";

    EXPECT_FALSE(parseFlagConfiguration(jsonString));
    EXPECT_TRUE(appenderContains("Failed to parse flag configuration, keeping existing settings"));
}

// Local Settings ================================================================

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsDefaultWhenFileDoenstExist)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(
        Throw(Common::FileSystem::IFileSystemException("Error, Failed to read file: pretend/file.txt, file does not exist")));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.numScanThreads, m_defaultThreads);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, defaultCacheAllEvents);
    EXPECT_EQ(result.uncacheDetections, defaultUncacheDetections);

    EXPECT_TRUE(appenderContains(m_localSettingsNotUsedMessage));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsFromEmptyFile)
{
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(1));
    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, sophos_on_access_process::OnAccessConfig::defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, sophos_on_access_process::OnAccessConfig::defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, sophos_on_access_process::OnAccessConfig::defaultCacheAllEvents);
    EXPECT_EQ(result.numScanThreads, 1); // 1 CPU core
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsLogsWhenSettingFromHardwareConcurrency)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_TRUE(appenderContains("Setting number of scanning threads from Hardware Concurrency: 5"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsLogsWhenCantSetFromHardwareConcurrency)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, "");
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(0));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_TRUE(appenderContains("Could not determine hardware concurrency using default of 10"));
}


TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsInvalidJsonSyntax)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, "this is going to break");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };

    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, defaultCacheAllEvents);
    EXPECT_EQ(result.uncacheDetections, defaultUncacheDetections);
    EXPECT_EQ(result.numScanThreads, m_defaultThreads);

    EXPECT_TRUE(appenderContains("Setting number of scanning threads from Hardware Concurrency: 5"));
    EXPECT_TRUE(appenderContains("Failed to read product config file info: "));
    EXPECT_TRUE(appenderContains(m_localSettingsNotUsedMessage));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsEmptyJSON)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, "{}");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, defaultCacheAllEvents);
    EXPECT_EQ(result.uncacheDetections, defaultUncacheDetections);
    EXPECT_EQ(result.numScanThreads, m_defaultThreads);

    EXPECT_TRUE(appenderContains("Setting number of scanning threads from Hardware Concurrency: 5"));
    EXPECT_TRUE(appenderContains(m_localSettingsNotUsedMessage));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsJsonOverridesHardwareConcurrency)
{
    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "numThreads" : 42
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 42);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsMinLimitQueue)
{
    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "maxscanqueuesize" : 38
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, 1000);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsMaxThreadCount)
{
    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "numThreads" : 999999
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 100);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsZeroCpu)
{
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(0));
    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, sophos_on_access_process::OnAccessConfig::defaultScanningThreads);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsTenCPUCores)
{
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 5);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsHardwareConcurrencyBypassesMaxThreads)
{
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(400));
    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 200);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsHardwareConcurrencySetsThreads)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto res = readLocalSettingsFile(m_sysCallWrapper);

    auto numScanThreads = (std::thread::hardware_concurrency() + 1) / 2;

    EXPECT_EQ(res.numScanThreads, numScanThreads);
    std::stringstream expectedMsg;
    expectedMsg << "Setting number of scanning threads from Hardware Concurrency: " << numScanThreads;
    EXPECT_TRUE(appenderContains(expectedMsg.str()));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsConfigEmptyKeepsDefaults)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, defaultCacheAllEvents);
    EXPECT_EQ(result.uncacheDetections, defaultUncacheDetections);
    EXPECT_EQ(result.numScanThreads, m_defaultThreads);

    EXPECT_TRUE(appenderContains(m_localSettingsNotUsedMessage));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsIgnoresBadvalues)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "dumpPerfData" : "ha",
        "cacheAllEvents" : true,
        "uncacheDetections" : false,
        "maxscanqueuesize" : "ha",
        "numThreads" : "ha"
    })");

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, true);
    EXPECT_EQ(result.uncacheDetections, false);
    EXPECT_EQ(result.numScanThreads, m_defaultThreads);

    EXPECT_TRUE(appenderContains("Setting dumpPerfData from file: false")); //boolean check for 'ha' returns false
    EXPECT_TRUE(appenderContains("cacheAllEvents from file: true"));
    EXPECT_TRUE(appenderContains("uncacheDetections from file: false"));
    EXPECT_TRUE(appenderContains("Failed to read product config file info: [json.exception.type_error.302] type must be number, but is string"));
    EXPECT_TRUE(appenderContains("Some or all local settings weren't set from file: Queue Size: 100000, Max threads: 5, Perf dump: false, Cache all events: true, Uncache detections: false"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsToProvidedValuesWhenFileExists)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "dumpPerfData" : true,
        "cacheAllEvents" : true,
        "uncacheDetections" : false,
        "maxscanqueuesize" : 2000,
        "numThreads" : 20
    })");

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, 2000);
    EXPECT_EQ(result.dumpPerfData, true);
    EXPECT_EQ(result.cacheAllEvents, true);
    EXPECT_EQ(result.uncacheDetections, false);
    EXPECT_EQ(result.numScanThreads, 20);

    EXPECT_FALSE(appenderContains("Some or all local settings weren't set from file:"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsToProvidedValuesWhenFileExistsDuplicateEntries)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "dumpPerfData" : true,
        "dumpPerfData" : false,
        "cacheAllEvents" : true,
        "cacheAllEvents" : false,
        "uncacheDetections" : false,
        "numThreads" : 40,
        "uncacheDetections" : true,
        "maxscanqueuesize" : 2000,
        "numThreads" : 20,
        "maxscanqueuesize" : 4000
    })");

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, 4000);
    EXPECT_EQ(result.dumpPerfData, false);
    EXPECT_EQ(result.cacheAllEvents, false);
    EXPECT_EQ(result.uncacheDetections, true);
    EXPECT_EQ(result.numScanThreads, 20);

    EXPECT_FALSE(appenderContains("Some or all local settings weren't set from file:"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettings_missingFields)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(R"({"numThreads": 20, "uncacheDetections": true})"));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, defaultCacheAllEvents);
    EXPECT_EQ(result.uncacheDetections, true);
    EXPECT_EQ(result.numScanThreads, 20);

    EXPECT_TRUE(appenderContains("Setting dumpPerfData from default: false"));
    EXPECT_TRUE(appenderContains("cacheAllEvents from default: false"));
    EXPECT_TRUE(appenderContains("Queue size from default: 100000"));

    EXPECT_TRUE(appenderContains("uncacheDetections from file: true"));
    EXPECT_TRUE(appenderContains("Scanning Thread count from file: 20"));

    EXPECT_FALSE(appenderContains("Some or all local settings weren't set from file:"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsToMaxPossibleValueWhenProvidedValuesToHigh)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const int aboveMaxThreads = maxAllowedScanningThreads + 1;
    const int aboveMaxQueue = maxAllowedQueueSize + 1;

    std::stringstream returnStr;
    returnStr << R"({"numThreads": )" << aboveMaxThreads << R"(, "maxscanqueuesize": )" << aboveMaxQueue << "}";

    expectReadConfig(*m_mockIFileSystemPtr, returnStr.str());

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, maxAllowedQueueSize);
    EXPECT_EQ(result.numScanThreads, maxAllowedScanningThreads);

    std::stringstream queueSizeLogMsg;
    queueSizeLogMsg << "Queue size of " << aboveMaxQueue << " is greater than maximum allowed of " << maxAllowedQueueSize;

    std::stringstream threadCountLogMsg;
    threadCountLogMsg << "Scanning Thread count of " << aboveMaxThreads << " is greater than maximum allowed of " << maxAllowedScanningThreads;

    EXPECT_TRUE(appenderContains("Setting Queue size from file: " + std::to_string(aboveMaxQueue)));
    EXPECT_TRUE(appenderContains("Setting Scanning Thread count from file: " + std::to_string(aboveMaxThreads)));
    EXPECT_TRUE(appenderContains(queueSizeLogMsg.str()));
    EXPECT_TRUE(appenderContains(threadCountLogMsg.str()));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsToMinPossibleValueWhenProvidedValuesToLow)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const int belowMinThreads = minAllowedScanningThreads - 1;
    const int belowMinQueue = minAllowedQueueSize - 1;

    std::stringstream returnStr;
    returnStr << R"({"numThreads": )" << belowMinThreads << R"(, "maxscanqueuesize": )" << belowMinQueue << "}";

    expectReadConfig(*m_mockIFileSystemPtr, returnStr.str());
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, minAllowedQueueSize);
    EXPECT_EQ(result.numScanThreads, minAllowedScanningThreads);

    std::stringstream queueSizeLogMsg;
    queueSizeLogMsg << "Queue size of " << belowMinQueue << " is less than minimum allowed of " << minAllowedQueueSize;

    std::stringstream threadCountLogMsg;
    threadCountLogMsg << "Scanning Thread count of " << belowMinThreads << " is less than minimum allowed of " << minAllowedScanningThreads;

    EXPECT_TRUE(appenderContains("Setting Queue size from file: " + std::to_string(belowMinQueue)));
    EXPECT_TRUE(appenderContains("Setting Scanning Thread count from file: " + std::to_string(belowMinThreads)));
    EXPECT_TRUE(appenderContains(queueSizeLogMsg.str()));
    EXPECT_TRUE(appenderContains(threadCountLogMsg.str()));
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

TEST_P(TestOnAccessConfigUtilsParameterized, readLocalSettingsEmpty_numberOfCoresDeterminesNumberOfScanningThreads)
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
