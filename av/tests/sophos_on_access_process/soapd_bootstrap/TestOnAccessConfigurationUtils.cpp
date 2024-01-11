// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "sophos_on_access_process/soapd_bootstrap/OnAccessConfigurationUtils.h"

#include "Common/FileSystem/IFileSystemException.h"

#include "../SoapMemoryAppenderUsingTests.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockSysCalls.h"

#include "tests/common/LogInitializedTests.h"

#include <gtest/gtest.h>

#include <fstream>
#include <tuple>

using namespace sophos_on_access_process::OnAccessConfig;
namespace fs = sophos_filesystem;

namespace
{
    std::string cacheAllEventsDefaultAsString()
    {
        return defaultCacheAllEvents?"true":"false";
    }

    std::string cacheAllEventsDefaultMessage()
    {
        std::ostringstream ost;
        ost << "cacheAllEvents from default: " << cacheAllEventsDefaultAsString();
        return ost.str();
    }

    std::string createLocalSettingsNotUsedMessage()
    {
        std::ostringstream ost;
        ost << "Some or all local settings weren't set from file: "
            << "Queue Size: 100000, Max threads: 5, Perf dump: false, Cache all events: " << cacheAllEventsDefaultAsString() << ", Uncache detections: true";
        return ost.str();
    }

    class TestOnAccessConfigurationUtils : public SoapMemoryAppenderUsingTests
    {
    public:
        TestOnAccessConfigurationUtils()
            :
                m_defaultTestExclusions{{"/mnt/", "/uk-filer5/"}}
        {}
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
            m_localSettingsNotUsedMessage = createLocalSettingsNotUsedMessage();
            m_binaryFile = std::string{"\x00\x72\x6c\x5f\x69\x6e\x69\x74\x69\x61\x6c\x69\x7a\x65\x5f\x66\x75\x6e\x6d\x61"
                                      "\x70\x00\x72\x6c\x5f\x63\x6f\x6d\x70\x6c\x65\x74\x69\x6f\x6e\x5f\x73\x75\x70\x70\x72"
                                      "\x65\x73\x73\x5f\x71\x75\x6f\x74\x65\x00\x73\x75\x62\x73\x68\x65\x6c\x6c\x5f", 120};
            m_sysCallWrapper = std::make_shared<Common::SystemCallWrapper::SystemCallWrapper>();
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
        std::string m_binaryFile;
        std::string m_oaPolicyConfigPath;
        std::string m_oaLocalSettingsPath;
        std::string m_oaFlagsPath;
        std::string m_localSettingsNotUsedMessage;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
        common::ExclusionList m_defaultTestExclusions;
        std::shared_ptr<Common::SystemCallWrapper::SystemCallWrapper> m_sysCallWrapper;
        std::shared_ptr<NiceMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
        //hardware_concurrency syscall default return value will result in defaultThreads below
        const int m_defaultThreads = 5;
    };

    void readLocalSettingsFile(size_t& maxScanQueueSize, int& numScanThreads, bool& dumpPerfData, const Common::SystemCallWrapper::ISystemCallWrapperSharedPtr & sysCalls)
    {
        auto settings = sophos_on_access_process::OnAccessConfig::readLocalSettingsFile(sysCalls);
        maxScanQueueSize = settings.maxScanQueueSize;
        numScanThreads = settings.numScanThreads;
        dumpPerfData = settings.dumpPerfData;
    }
}
// Policy config ==================================================================

TEST_F(TestOnAccessConfigurationUtils, readPolicyNotAFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories(m_testDir /= "var/on_access_policy.json/");

    EXPECT_EQ(readPolicyConfigFile(), "");

    EXPECT_TRUE(appenderContains("Failed to read on-access configuration, keeping existing configuration:"));
    EXPECT_TRUE(appenderContains("is a directory"));
}

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
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"],"detectPUAs":false})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions = m_defaultTestExclusions;
    expectedResult.detectPUAs = false;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);

    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = m_defaultTestExclusions;
    expectedResult.detectPUAs = true;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_NE(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsHandlesBinary)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    OnAccessConfiguration expectedResult{};

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(m_binaryFile, m_testConfig), false);
    EXPECT_EQ(m_testConfig, expectedResult);

    EXPECT_TRUE(appenderContains("Failed to parse json configuration due to parse error, reason:"));
    EXPECT_TRUE(appenderContains("Failed to parse json configuration, keeping existing settings"));
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJsonDuplicates)
{
    std::string jsonString = R"({
        "enabled":"true",
        "excludeRemoteFiles":"false",
        "enabled":"false",
        "excludeRemoteFiles":"true",
        "exclusions":["/tmp/","/uk-filer6/"],
        "exclusions":["/mnt/","/uk-filer5/"],
        "detectPUAs":"false"
        })";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = m_defaultTestExclusions;
    expectedResult.detectPUAs = false;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsManyExclusions)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::stringstream exclusionStr;
    std::vector<std::string> expectedExclusion;
    exclusionStr << R"({"exclusions":[)";

    for (uint excCount = 0; excCount < 400; excCount ++)
    {
        exclusionStr << R"("/tmp)" << excCount << R"(/",)";
        expectedExclusion.emplace_back("/tmp" + std::to_string(excCount) + "/");
    }

    exclusionStr.seekp(-1, std::ios_base::cur);
    exclusionStr << "]}";

    const common::ExclusionList expectedExclusionList{expectedExclusion};

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(exclusionStr.str(), m_testConfig), true);
    EXPECT_EQ(m_testConfig.exclusions, expectedExclusionList);

    EXPECT_FALSE(appenderContains("Failed to parse json configuration, keeping existing settings"));
    EXPECT_TRUE(appenderContains(R"(On-access exclusions: ["/tmp0/","/tmp1/","/tmp2/","/tmp3/","/tmp4/")"));
}


TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson_missingExcludeRemote)
{
    std::string jsonString = R"({"enabled":"true","exclusions":["/mnt/","/uk-filer5/"],"detectPUAs":"false"})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions = m_defaultTestExclusions;
    expectedResult.detectPUAs = false;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson_missingEnabled)
{
    std::string jsonString = R"({"excludeRemoteFiles":"true","exclusions":["/mnt/","/uk-filer5/"],"detectPUAs":"false"})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = m_defaultTestExclusions;
    expectedResult.detectPUAs = false;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJson_missingExclusions)
{
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"true","detectPUAs":"false"})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.detectPUAs = false;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
}


TEST_F(TestOnAccessConfigurationUtils, enablesOnAccessWithNumberInField)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"enabled":123})";

    OnAccessConfiguration expectedResult{};
    expectedResult.enabled = true; //is set to true if non-zero
    expectedResult.detectPUAs = true;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);

    EXPECT_TRUE(appenderContains("Setting excludeRemoteFiles from default: false"));
    EXPECT_TRUE(appenderContains("Setting enabled from file: true"));
}

TEST_F(TestOnAccessConfigurationUtils, excludesRemoteFilesWithNumberInField)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"excludeRemoteFiles":123})";

    OnAccessConfiguration expectedResult{};
    expectedResult.excludeRemoteFiles = true; //is set to true if non-zero
    expectedResult.detectPUAs = true;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);

    EXPECT_TRUE(appenderContains("Setting excludeRemoteFiles from file: true"));
    EXPECT_TRUE(appenderContains("Setting enabled from default: false"));
}

TEST_F(TestOnAccessConfigurationUtils, enabledFieldReadWithStringInField)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"enabled":"true"})";

    OnAccessConfiguration expectedResult{};
    expectedResult.excludeRemoteFiles = true;
    expectedResult.enabled = false;
    expectedResult.detectPUAs = true;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    //ToBoolean interprets string as being equal to "true" or not
    EXPECT_TRUE(appenderContains("Setting enabled from file: true"));
}

TEST_F(TestOnAccessConfigurationUtils, excludesRemoteFilesReadWithStringInField)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"excludeRemoteFiles":"true"})";

    OnAccessConfiguration expectedResult{};
    expectedResult.excludeRemoteFiles = true;
    expectedResult.enabled = false;
    expectedResult.detectPUAs = true;

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    EXPECT_EQ(m_testConfig, expectedResult);
    //ToBoolean interprets string as being equal to "true" or not
    EXPECT_TRUE(appenderContains("Setting excludeRemoteFiles from file: true"));
}

TEST_F(TestOnAccessConfigurationUtils, processesExclusionsWithNumberInField)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"exclusions":[1,2]})";

    OnAccessConfiguration expectedResult{};

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), false);
    EXPECT_EQ(m_testConfig, expectedResult);
    EXPECT_TRUE(appenderContains("Failed to parse json configuration due to type error"));
    EXPECT_TRUE(appenderContains("Failed to parse json configuration, keeping existing settings"));
}

TEST_F(TestOnAccessConfigurationUtils, processEnabledWithMixedCharacters)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"enabled":24311ddaf53, "excludeRemoteFiles":true}})";

    OnAccessConfiguration expectedResult{};

    EXPECT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), false);

    EXPECT_EQ(m_testConfig, expectedResult);
    EXPECT_TRUE(appenderContains("Failed to parse json configuration due to parse error"));
    EXPECT_TRUE(appenderContains("Failed to parse json configuration, keeping existing settings"));
}

TEST_F(TestOnAccessConfigurationUtils, processExcludeRemoteWithMixedCharacters)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"({"enabled":true, "excludeRemoteFiles":24311ddaf53})";

    OnAccessConfiguration expectedResult{};

    EXPECT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), false);

    EXPECT_EQ(m_testConfig, expectedResult);
    EXPECT_TRUE(appenderContains("Failed to parse json configuration due to parse error"));
    EXPECT_TRUE(appenderContains("Failed to parse json configuration, keeping existing settings"));
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessSettingsFromJsonInvalidJsonSyntax)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string jsonString = R"(this is going to break)";

    OnAccessConfiguration expectedResult {};

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), false);
    EXPECT_EQ(m_testConfig, expectedResult);
    EXPECT_TRUE(appenderContains("Failed to parse json configuration, keeping existing settings"));
}

TEST_F(TestOnAccessConfigurationUtils, parseOnAccessPolicySettingsFromJsonSortsExclusions)
{
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt*foo/","/uk-filer5/"],"detectPUAs":false})";

    ASSERT_EQ(parseOnAccessPolicySettingsFromJson(jsonString, m_testConfig), true);
    ASSERT_EQ(m_testConfig.exclusions.size(), 2);
    EXPECT_LT(m_testConfig.exclusions[0], m_testConfig.exclusions[1]);
    EXPECT_EQ(m_testConfig.exclusions[0].type(), ExclusionType::STEM);
    EXPECT_EQ(m_testConfig.exclusions[0].displayPath(), "/uk-filer5/");
}

TEST_F(TestOnAccessConfigurationUtils, onOpenTrue)
{
    ASSERT_TRUE(parseOnAccessPolicySettingsFromJson(R"({"onOpen":true})", m_testConfig));
    EXPECT_TRUE(m_testConfig.onOpen);
}

TEST_F(TestOnAccessConfigurationUtils, onOpenTrueString)
{
    ASSERT_TRUE(parseOnAccessPolicySettingsFromJson(R"({"onOpen":"true"})", m_testConfig));
    EXPECT_TRUE(m_testConfig.onOpen);
}

TEST_F(TestOnAccessConfigurationUtils, onOpenNumber)
{
    ASSERT_TRUE(parseOnAccessPolicySettingsFromJson(R"({"onOpen":43})", m_testConfig));
    EXPECT_TRUE(m_testConfig.onOpen);
}

TEST_F(TestOnAccessConfigurationUtils, onOpenZero)
{
    ASSERT_TRUE(parseOnAccessPolicySettingsFromJson(R"({"onOpen":0})", m_testConfig));
    EXPECT_FALSE(m_testConfig.onOpen);
}

TEST_F(TestOnAccessConfigurationUtils, onOpenFalse)
{
    ASSERT_TRUE(parseOnAccessPolicySettingsFromJson(R"({"onOpen":false})", m_testConfig));
    EXPECT_FALSE(m_testConfig.onOpen);
}

TEST_F(TestOnAccessConfigurationUtils, onCloseTrue)
{
    ASSERT_TRUE(parseOnAccessPolicySettingsFromJson(R"({"onClose":true})", m_testConfig));
    EXPECT_TRUE(m_testConfig.onClose);
}

TEST_F(TestOnAccessConfigurationUtils, onCloseFalse)
{
    ASSERT_TRUE(parseOnAccessPolicySettingsFromJson(R"({"onClose":false})", m_testConfig));
    EXPECT_FALSE(m_testConfig.onClose);
}

// Local Settings ================================================================


TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsDefaultWhenNotAFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories(m_testDir /= "var/on_access_local_settings.json/");
    auto result = readLocalSettingsFile(m_sysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_TRUE(appenderContains("Local Settings file could not be read: Error, Failed to read file:"));
    EXPECT_TRUE(appenderContains("is a directory"));
    EXPECT_TRUE(appenderContains("Some or all local settings weren't set from file:"));
}

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
    EXPECT_TRUE(appenderContains("Local Settings file could not be read: Error, Failed to read file: pretend/file.txt, file does not exist"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsFromEmptyFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    //Set to 8 as we dont want to exercise hardware concurrency minimum limit code
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(8));
    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, defaultCacheAllEvents);
    EXPECT_EQ(result.numScanThreads, 4);

    EXPECT_TRUE(appenderContains("Local Settings file is empty"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsLogsWhenSettingFromHardwareConcurrency)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_TRUE(appenderContains("Setting number of scanning threads from Hardware Concurrency: 5"));
}

TEST_F(TestOnAccessConfigurationUtils, numberOfThreadsSetByHardwareConcurrencyIsLimitedToUpperBound40)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(100));

    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_TRUE(appenderContains("Hardware concurrency result is 50 which is to high. Reducing number of threads to " + std::to_string(maxConcurrencyScanningThreads)));
    EXPECT_EQ(result.numScanThreads, 40);
}

TEST_F(TestOnAccessConfigurationUtils, numberOfThreadsSetByHardwareConcurrencyIsLimitedToLowerBound4)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(4));

    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_TRUE(appenderContains("Hardware concurrency result is 2 which is to low. Increasing number of threads to " + std::to_string(minConcurrencyScanningThreads)));
    EXPECT_EQ(result.numScanThreads, 4);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsLogsWhenCantSetFromHardwareConcurrency)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, "");
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(0));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    readLocalSettingsFile(m_mockSysCallWrapper);

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
    EXPECT_EQ(result.highPrioritySoapd, defaultHighPrioritySoapd);
    EXPECT_EQ(result.highPriorityThreatDetector, defaultHighPriorityThreatDetector);
    EXPECT_EQ(result.uncacheDetections, defaultUncacheDetections);
    EXPECT_EQ(result.numScanThreads, m_defaultThreads);

    EXPECT_TRUE(appenderContains("Setting number of scanning threads from Hardware Concurrency: 5"));
    EXPECT_TRUE(appenderContains("Failed to read local settings: "));
    EXPECT_TRUE(appenderContains(m_localSettingsNotUsedMessage));
}

TEST_F(TestOnAccessConfigurationUtils, jsonOverrides)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    expectReadConfig(*filesystemMock, R"({
        "dumpPerfData" : true,
        "cacheAllEvents" : true,
        "uncacheDetections" : false,
        "maxscanqueuesize" : 1038,
        "highPrioritySoapd" : true,
        "highPriorityThreatDetector" : true,
        "numThreads" : 42
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, 1038);
    EXPECT_TRUE(result.dumpPerfData);
    EXPECT_TRUE(result.cacheAllEvents);
    EXPECT_TRUE(result.highPriorityThreatDetector);
    EXPECT_TRUE(result.highPrioritySoapd);
    EXPECT_FALSE(result.uncacheDetections);
    EXPECT_EQ(result.numScanThreads, 42);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsBinary)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_oaLocalSettingsPath)).WillOnce(Return(m_binaryFile));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };

    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, defaultDumpPerfData);
    EXPECT_EQ(result.cacheAllEvents, defaultCacheAllEvents);
    EXPECT_EQ(result.uncacheDetections, defaultUncacheDetections);
    EXPECT_EQ(result.numScanThreads, m_defaultThreads);

    EXPECT_TRUE(appenderContains("Failed to read local settings: [json.exception.parse_error.101] parse error at line 1"));
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

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsJsonSuccessfullyMeansWeDontCheckConcurrency)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "numThreads" : 99
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);

    EXPECT_EQ(result.numScanThreads, 99);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsJsonIsNotOverridenByConcurrencyMaximum)
{
    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "numThreads" : 100
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 100);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsWithNoThreadFieldSetsFromConcurrency)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(8));
    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "maxscanqueuesize" : 38
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 4);
    EXPECT_TRUE(waitForLog("Setting number of scanning threads from Hardware Concurrency"));
}


TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsMinLimitQueue)
{
    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "maxscanqueuesize" : 38
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.maxScanQueueSize, 1000);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsMaxThreadCount)
{
    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "numThreads" : 999999
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 100);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsZeroCpu)
{
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(0));
    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, defaultScanningThreads);
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsTenCPUCores)
{
    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*m_mockIFileSystemPtr, "");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockIFileSystemPtr) };
    auto result = readLocalSettingsFile(m_mockSysCallWrapper);
    EXPECT_EQ(result.numScanThreads, 5);
}

TEST_F(TestOnAccessConfigurationUtils, JustHighPrioritySoapdTrue)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, R"({
        "highPrioritySoapd" : true
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.highPriorityThreatDetector, defaultHighPriorityThreatDetector);
    EXPECT_TRUE(result.highPrioritySoapd);
}

TEST_F(TestOnAccessConfigurationUtils, JustHighPrioritySoapdFalse)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, R"({
        "highPrioritySoapd" : false
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.highPriorityThreatDetector, defaultHighPriorityThreatDetector);
    EXPECT_FALSE(result.highPrioritySoapd);
}

TEST_F(TestOnAccessConfigurationUtils, JustHighPriorityThreatDetectorTrue)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, R"({
        "highPriorityThreatDetector" : true
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.highPrioritySoapd, defaultHighPrioritySoapd);
    EXPECT_TRUE(result.highPriorityThreatDetector);
}

TEST_F(TestOnAccessConfigurationUtils, JustHighPriorityThreatDetectorFalse)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*sysCallWrapper, hardware_concurrency()).WillOnce(Return(10));
    expectReadConfig(*filesystemMock, R"({
        "highPriorityThreatDetector" : false
    })");

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock) };
    auto result = readLocalSettingsFile(sysCallWrapper);
    EXPECT_EQ(result.highPrioritySoapd, defaultHighPrioritySoapd);
    EXPECT_FALSE(result.highPriorityThreatDetector);
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

    EXPECT_TRUE(appenderContains("Setting dumpPerfData from file: false")); //boolean check for string != "true" returns false
    EXPECT_TRUE(appenderContains("cacheAllEvents from file: true"));
    EXPECT_TRUE(appenderContains("uncacheDetections from file: false"));
    EXPECT_TRUE(appenderContains("Failed to read local settings: [json.exception.type_error.302] type must be number, but is string"));
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

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsToDefaultWithWrongTypes)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    expectReadConfig(*m_mockIFileSystemPtr, R"({
        "dumpPerfData" : "wrong",
        "cacheAllEvents" : "incorrect",
        "uncacheDetections" : "invalid",
        "maxscanqueuesize" : "iamnotanumber",
        "numThreads" : "stillnotanumber"
    })");

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto numScanThreads = (std::thread::hardware_concurrency() + 1) / 2;
    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, defaultMaxScanQueueSize);
    EXPECT_EQ(result.dumpPerfData, false); //boolean check for string != "true" returns false
    EXPECT_EQ(result.cacheAllEvents, false); //boolean check for string != "true" returns false
    EXPECT_EQ(result.uncacheDetections, false); //boolean check for string != "true" returns false
    EXPECT_EQ(result.numScanThreads, numScanThreads);

    EXPECT_TRUE(appenderContains("Setting number of scanning threads from Hardware Concurrency: " + std::to_string(numScanThreads)));
    EXPECT_TRUE(appenderContains("Setting dumpPerfData from file: false"));
    EXPECT_TRUE(appenderContains("Setting cacheAllEvents from file: false"));
    EXPECT_TRUE(appenderContains("Setting uncacheDetections from file: false"));
    EXPECT_TRUE(appenderContains("Failed to read local settings: [json.exception.type_error.302] type must be number, but is string"));
    EXPECT_TRUE(appenderContains("Some or all local settings weren't set from file:"));
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
    EXPECT_TRUE(appenderContains(cacheAllEventsDefaultMessage()));
    EXPECT_TRUE(appenderContains("Queue size from default: 100000"));

    EXPECT_TRUE(appenderContains("uncacheDetections from file: true"));
    EXPECT_TRUE(appenderContains("Scanning Thread count from file: 20"));

    EXPECT_FALSE(appenderContains("Some or all local settings weren't set from file:"));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsToMaxPossibleValueWhenProvidedValuesToHigh)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const int aboveMaxThreads = maxConfigurableScanningThreads + 1;
    const int aboveMaxQueue = maxAllowedQueueSize + 1;

    std::stringstream returnStr;
    returnStr << R"({"numThreads": )" << aboveMaxThreads << R"(, "maxscanqueuesize": )" << aboveMaxQueue << "}";

    expectReadConfig(*m_mockIFileSystemPtr, returnStr.str());

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, maxAllowedQueueSize);
    EXPECT_EQ(result.numScanThreads, maxConfigurableScanningThreads);

    std::stringstream queueSizeLogMsg;
    queueSizeLogMsg << "Queue size of " << aboveMaxQueue << " is greater than maximum allowed of " << maxAllowedQueueSize;

    std::stringstream threadCountLogMsg;
    threadCountLogMsg << "Scanning Thread count of " << aboveMaxThreads << " is greater than maximum allowed of " << maxConfigurableScanningThreads;

    EXPECT_TRUE(appenderContains("Setting Queue size from file: " + std::to_string(aboveMaxQueue)));
    EXPECT_TRUE(appenderContains("Setting Scanning Thread count from file: " + std::to_string(aboveMaxThreads)));
    EXPECT_TRUE(appenderContains(queueSizeLogMsg.str()));
    EXPECT_TRUE(appenderContains(threadCountLogMsg.str()));
}

TEST_F(TestOnAccessConfigurationUtils, readLocalSettingsSetsToMinPossibleValueWhenProvidedValuesToLow)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const int belowMinThreads = minConfigurableScanningThreads - 1;
    const int belowMinQueue = minAllowedQueueSize - 1;

    std::stringstream returnStr;
    returnStr << R"({"numThreads": )" << belowMinThreads << R"(, "maxscanqueuesize": )" << belowMinQueue << "}";

    expectReadConfig(*m_mockIFileSystemPtr, returnStr.str());
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto result = readLocalSettingsFile(m_sysCallWrapper);

    EXPECT_EQ(result.maxScanQueueSize, minAllowedQueueSize);
    EXPECT_EQ(result.numScanThreads, minConfigurableScanningThreads);

    std::stringstream queueSizeLogMsg;
    queueSizeLogMsg << "Queue size of " << belowMinQueue << " is less than minimum allowed of " << minAllowedQueueSize;

    std::stringstream threadCountLogMsg;
    threadCountLogMsg << "Scanning Thread count of " << belowMinThreads << " is less than minimum allowed of " << minConfigurableScanningThreads;

    EXPECT_TRUE(appenderContains("Setting Queue size from file: " + std::to_string(belowMinQueue)));
    EXPECT_TRUE(appenderContains("Setting Scanning Thread count from file: " + std::to_string(belowMinThreads)));
    EXPECT_TRUE(appenderContains(queueSizeLogMsg.str()));
    EXPECT_TRUE(appenderContains(threadCountLogMsg.str()));
}

class TestOnAccessConfigUtilsParameterized
    : public ::testing::TestWithParam<std::pair<unsigned int, int>>
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
        std::make_pair(1, 4),
        std::make_pair(2, 4),
        std::make_pair(3, 4),
        std::make_pair(8, 4),
        std::make_pair(15, 8),
        std::make_pair(16, 8),
        std::make_pair(17, 9),
        std::make_pair(17, 9),
        std::make_pair(78, 39),
        std::make_pair(80, 40),
        std::make_pair(99, 40),
        std::make_pair(100, 40),
        std::make_pair(101, 40)
            ));

TEST_P(TestOnAccessConfigUtilsParameterized, readLocalSettingsEmpty_numberOfCoresDeterminesNumberOfScanningThreads)
{
    auto [ concurrency, numScanThreads ] = GetParam();

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    size_t maxScanQueueItems = 0;
    int numThreads = 0;
    bool dumpPerfData = true;

    EXPECT_CALL(*m_mockSysCallWrapper, hardware_concurrency()).WillOnce(Return(concurrency));
    readLocalSettingsFile(maxScanQueueItems, numThreads, dumpPerfData, m_mockSysCallWrapper);

    EXPECT_EQ(maxScanQueueItems, defaultMaxScanQueueSize);
    EXPECT_EQ(numThreads, numScanThreads);
    EXPECT_FALSE(dumpPerfData);
}
