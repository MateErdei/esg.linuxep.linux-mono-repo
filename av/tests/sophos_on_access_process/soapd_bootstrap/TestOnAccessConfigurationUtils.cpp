// Copyright 2022 Sophos Limited. All rights reserved.

#include "sophos_on_access_process/soapd_bootstrap/OnAccessConfigurationUtils.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include "tests/common/LogInitializedTests.h"
#include "tests/datatypes/MockSysCalls.h"

#include <gtest/gtest.h>

#include <tuple>

namespace
{
    class TestOnAccessConfigurationUtils : public LogInitializedTests
    {
    public:
        const std::string PLUGIN_INSTALL = "/tmp/TestOnAccessConfigurationUtils";
        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", PLUGIN_INSTALL);
        }

        void expectReadConfig(MockFileSystem& mock, const std::string& contents)
        {
            const std::string path = PLUGIN_INSTALL + "/var/on_access_local_settings.json";
            EXPECT_CALL(mock, readFile(path)).WillOnce(Return(contents));
        }
    };
}

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
