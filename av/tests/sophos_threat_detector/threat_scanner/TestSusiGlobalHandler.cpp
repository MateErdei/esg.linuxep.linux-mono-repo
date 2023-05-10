// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

#include "MockSusiApi.h"

#include "../../common/LogInitializedTests.h"
#include "common/MemoryAppender.h"
#include "sophos_threat_detector/threat_scanner/SusiGlobalHandler.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace  threat_scanner;

namespace
{
    class TestSusiGlobalHandler : public MemoryAppenderUsingTests
    {
    public:
        TestSusiGlobalHandler() : MemoryAppenderUsingTests("ThreatScanner") {}
    };
}

TEST_F(TestSusiGlobalHandler, testConstructor)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    ASSERT_NO_THROW(auto temp = SusiGlobalHandler(mockSusiApi));
}

TEST_F(TestSusiGlobalHandler, testBootstrapping)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);
    ASSERT_EQ(SUSI_S_OK, globalHandler.bootstrap());
}

TEST_F(TestSusiGlobalHandler, testBootstrappingFails)
{
    auto mockSusiApi = std::make_shared<StrictMock<MockSusiApi>>();

    EXPECT_CALL(*mockSusiApi, SUSI_SetLogCallback(_)).WillRepeatedly(Return(SUSI_S_OK));

    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_CALL(*mockSusiApi, SUSI_Install(_,_)).WillOnce(Return(SUSI_E_INTERNAL));

    ASSERT_EQ(SUSI_E_INTERNAL, globalHandler.bootstrap());
}

TEST_F(TestSusiGlobalHandler, testInitializeSusi)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    std::string resultJson;
    SusiVersionResult result = { .version = 1, .versionResultJson = resultJson.data() };

    EXPECT_CALL(*mockSusiApi, SUSI_GetVersion(_)).WillOnce(::testing::DoAll(testing::SetArgPointee<0>(&result), Return(SUSI_S_OK)));
    ASSERT_EQ(true, globalHandler.initializeSusi(""));
}

TEST_F(TestSusiGlobalHandler, testInitializeSusiFirstBootStrapFails)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_CALL(*mockSusiApi, SUSI_Install(_,_)).WillOnce(Return(SUSI_E_INTERNAL));
    EXPECT_THROW(globalHandler.initializeSusi(""), std::runtime_error);
}

TEST_F(TestSusiGlobalHandler, testInitializeSusiFirstInitFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    std::string resultJson;
    SusiVersionResult result = { .version = 1, .versionResultJson = resultJson.data() };

    EXPECT_CALL(*mockSusiApi, SUSI_Initialize(_,_))
        .WillOnce(Return(SUSI_E_INTERNAL)).
        WillOnce(Return(SUSI_S_OK));
    EXPECT_CALL(*mockSusiApi, SUSI_GetVersion(_))
        .WillOnce(::testing::DoAll(testing::SetArgPointee<0>(&result), Return(SUSI_S_OK)));
    ASSERT_EQ(true, globalHandler.initializeSusi(""));

    EXPECT_TRUE(appenderContains("Bootstrapping SUSI from update source: \"/susi/update_source\"", 2));
    EXPECT_TRUE(appenderContains("Attempting to re-install SUSI"));
    EXPECT_TRUE(appenderContains("Initialising Global Susi successful", 1));
}

TEST_F(TestSusiGlobalHandler, testInitializeSusiAllInitFail)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_CALL(*mockSusiApi, SUSI_Initialize(_,_)).WillRepeatedly(Return(SUSI_E_INTERNAL));
    EXPECT_THROW(globalHandler.initializeSusi(""), std::runtime_error);
    EXPECT_TRUE(appenderContains("Bootstrapping SUSI from update source: \"/susi/update_source\"", 2));
    EXPECT_FALSE(appenderContains("Initialising Global Susi successful"));
}

TEST_F(TestSusiGlobalHandler, isMachineLearningEnabled)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_TRUE(globalHandler.isMachineLearningEnabled());
}

TEST_F(TestSusiGlobalHandler, readMachineLearningFromEmptySettings)
{
    // Set PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    // Mock filesystem
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile("/etc/sophos_susi_force_machine_learning")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/plugin/var/susi_startup_settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/plugin/var/susi_startup_settings.json")).WillOnce(Return("{}"));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_TRUE(globalHandler.isMachineLearningEnabled());
}

TEST_F(TestSusiGlobalHandler, readMachineLearningFromFalseSettings)
{
    // Set PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    // Mock filesystem
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile("/etc/sophos_susi_force_machine_learning")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/plugin/var/susi_startup_settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/plugin/var/susi_startup_settings.json")).WillOnce(Return(R"({"machineLearning":false})"));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_FALSE(globalHandler.isMachineLearningEnabled());
}

TEST_F(TestSusiGlobalHandler, readMachineLearningFromOverride)
{
    // Set PLUGIN_INSTALL
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    // Mock filesystem
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile("/etc/sophos_susi_force_machine_learning")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, isFile("/plugin/var/susi_startup_settings.json")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/plugin/var/susi_startup_settings.json")).WillOnce(Return(R"({"machineLearning":false})"));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_TRUE(globalHandler.isMachineLearningEnabled());
}

TEST_F(TestSusiGlobalHandler, isBlockListedReturnsFalse)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_FALSE(globalHandler.IsBlocklistedFile(&globalHandler, SUSI_SHA256_ALG, "checksum", 8));
}

TEST_F(TestSusiGlobalHandler, isAllowListedFile_ReturnsFalse_IfAlgorithmIsNotSHA256)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_FALSE(globalHandler.isAllowlistedFile(&globalHandler, SUSI_SHA1_ALG, "checksum", 8));
    EXPECT_TRUE(appenderContains("isAllowlistFile called with unsupported algorithm: 2"));
}

TEST_F(TestSusiGlobalHandler, isAllowListedFile_ReturnsTrue_IfSusiSettingsAllowLists)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    UsingMemoryAppender memoryAppenderHolder(*this);

    auto susiSettings = std::make_shared<common::ThreatDetector::SusiSettings>();
    susiSettings->setAllowListSha256(std::vector<std::string>{"616c6c6f77656453"});

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);
    globalHandler.setSusiSettings(std::move(susiSettings));

    EXPECT_TRUE(globalHandler.isAllowlistedFile(&globalHandler, SUSI_SHA256_ALG, "allowedSha", 8));
    EXPECT_TRUE(appenderContains("Allowed by SHA256: 616c6c6f77656453"));
}

TEST_F(TestSusiGlobalHandler, isAllowListedFile_ReturnsFalse_IfSusiSettingsNotAllowLists)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    UsingMemoryAppender memoryAppenderHolder(*this);
    getLogger().setLogLevel(log4cplus::TRACE_LOG_LEVEL);
    auto susiSettings = std::make_shared<common::ThreatDetector::SusiSettings>();
    susiSettings->setAllowListSha256(std::vector<std::string>{"NotAllowed"});

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);
    globalHandler.setSusiSettings(std::move(susiSettings));

    EXPECT_FALSE(globalHandler.isAllowlistedFile(&globalHandler, SUSI_SHA256_ALG, "allowedSha", 8));
    EXPECT_TRUE(appenderContains("Denied allow list by SHA256 for: 616c6c6f77656453"));
}

TEST_F(TestSusiGlobalHandler, isAllowListedPath_ReturnsFalse_IfFilePathIsNullptr)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");

    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_FALSE(globalHandler.IsAllowlistedPath(&globalHandler, nullptr));
    EXPECT_TRUE(appenderContains("Allow list by path not possible, filePath provided by SUSI is invalid"));
}

TEST_F(TestSusiGlobalHandler, isAllowListedPath_ReturnsTrue_IfSusiSettingsAllowLists)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");
    const std::string allowedPath = "/AllowedPath";

    UsingMemoryAppender memoryAppenderHolder(*this);
    auto susiSettings = std::make_shared<common::ThreatDetector::SusiSettings>();
    susiSettings->setAllowListPath(std::vector<std::string>{allowedPath});

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);
    globalHandler.setSusiSettings(std::move(susiSettings));

    EXPECT_TRUE(globalHandler.IsAllowlistedPath(&globalHandler, allowedPath.c_str()));
    EXPECT_TRUE(appenderContains("Allowed by path: " + allowedPath));
}


TEST_F(TestSusiGlobalHandler, isAllowListedPath_ReturnsFalse_IfSusiSettingsAllowLists)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/plugin");
    const std::string allowedPath = "/AllowedPath";

    UsingMemoryAppender memoryAppenderHolder(*this);
    getLogger().setLogLevel(log4cplus::TRACE_LOG_LEVEL);
    auto susiSettings = std::make_shared<common::ThreatDetector::SusiSettings>();
    susiSettings->setAllowListPath(std::vector<std::string>{"NotAllowed"});

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);
    globalHandler.setSusiSettings(std::move(susiSettings));

    EXPECT_FALSE(globalHandler.IsAllowlistedPath(&globalHandler, allowedPath.c_str()));
    EXPECT_TRUE(appenderContains("Denied allow list by path for: " + allowedPath));
}