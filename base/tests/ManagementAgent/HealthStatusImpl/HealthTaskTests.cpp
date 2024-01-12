// Copyright 2021-2024 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/HealthTask/HealthTask.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/ManagementAgent/MockPluginManager/MockPluginManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    class HealthTaskTests : public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_healthStatus = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();

            auto xml = m_healthStatus->generateHealthStatusXml();
            EXPECT_TRUE(xml.hasStatusChanged);
        }

        const std::string m_tempDir = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        const std::string m_statusFilePath = Common::ApplicationConfiguration::applicationPathManager().getShsStatusFilePath();
        const std::string m_healthFilePath = Common::ApplicationConfiguration::applicationPathManager().getOverallHealthFilePath();
        const std::string m_healthDir = Common::ApplicationConfiguration::applicationPathManager().getInternalPolicyFilePath();
        const mode_t m_statusFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

        StrictMock<MockPluginManager> m_mockPluginManager;
        std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> m_healthStatus;

    private:
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    };
}

TEST_F(HealthTaskTests, run_healthStatusMessageIsUpdatedWhenDifferentFromCachedValue)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
        std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string expectedXml = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="admin" value="1" /><item name="service" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>)";
    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXml, m_tempDir, m_statusFileMode)).Times(1);
    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_healthFilePath, _, m_tempDir, m_statusFileMode)).Times(1);
    EXPECT_CALL(*filesystemMock, isFile(m_healthFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(m_healthFilePath)).WillOnce(Return("{}"));
    EXPECT_CALL(
        *filesystemMock,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(false));
    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).WillOnce(Return(std::vector<std::string>{}));
    auto healthStatus = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    EXPECT_CALL(m_mockPluginManager, getSharedHealthStatusObj()).WillRepeatedly(Return(healthStatus));

    ManagementAgent::HealthStatusImpl::HealthTask task(m_mockPluginManager);
    task.run();
}

TEST_F(HealthTaskTests, run_healthStatusMessageIsNotUpdatedWhenStatusValueHasNotChanged)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
        std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string expectedXml = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="admin" value="1" /><item name="service" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>)";
    EXPECT_CALL(*filesystemMock, isFile(m_healthFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(m_healthFilePath)).WillRepeatedly(Return("{\"health\":1,\"service\":1,\"threatService\":1,\"threat\":1}"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXml, m_tempDir, m_statusFileMode)).Times(1);
    EXPECT_CALL(
        *filesystemMock,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(false));
    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).Times(2).WillRepeatedly(Return(std::vector<std::string>{}));
    auto healthStatus = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    EXPECT_CALL(m_mockPluginManager, getSharedHealthStatusObj()).WillRepeatedly(Return(healthStatus));

    ManagementAgent::HealthStatusImpl::HealthTask task1(m_mockPluginManager);
    task1.run();

    ManagementAgent::HealthStatusImpl::HealthTask task2(m_mockPluginManager);
    task2.run();
}

TEST_F(HealthTaskTests, run_healthStatusMessageIsUpdatedWhenStatusFileFailsToWriteSuccessfully)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
        std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string expectedXmlGood = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="admin" value="1" /><item name="service" value="1" ><detail name="Sophos MCS Client" value="0" /><detail name="Plugin One" value="0" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>)";
    std::string expectedXmlBad = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="3" /><item name="admin" value="1" /><item name="service" value="3" ><detail name="Sophos MCS Client" value="0" /><detail name="Plugin One" value="1" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>)";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthGood;
    pluginHealthGood.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginHealthGood.healthValue = 0;
    pluginHealthGood.displayName = "Plugin One";


    ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthBad = pluginHealthGood;
    pluginHealthBad.healthValue = 1;
    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_healthFilePath, _, m_tempDir, m_statusFileMode)).Times(1);
    EXPECT_CALL(*filesystemMock, isFile(m_healthFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(m_healthFilePath)).WillRepeatedly(Return("{\"health\":1,\"service\":1,\"threatService\":1,\"threat\":1}"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXmlBad, m_tempDir, m_statusFileMode)).Times(1).RetiresOnSaturation();

    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXmlBad, m_tempDir, m_statusFileMode)).
        WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST"))).RetiresOnSaturation();

    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXmlGood, m_tempDir, m_statusFileMode)).Times(1);
    EXPECT_CALL(
        *filesystemMock,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(false));
    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).Times(3).WillRepeatedly(Return(std::vector<std::string>{"pluginone"}));
    EXPECT_CALL(m_mockPluginManager, getHealthStatusForPlugin("pluginone", false)).Times(2).WillRepeatedly(Return(pluginHealthBad));
    EXPECT_CALL(m_mockPluginManager, getHealthStatusForPlugin("pluginone", false)).WillOnce(Return(pluginHealthGood)).RetiresOnSaturation();
    auto healthStatus = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    EXPECT_CALL(m_mockPluginManager, getSharedHealthStatusObj()).WillRepeatedly(Return(healthStatus));

    // Status Updated, Cached and file written
    ManagementAgent::HealthStatusImpl::HealthTask task1(m_mockPluginManager);
    task1.run();
    // Status Updated, Cached and file written results in ERROR
    ManagementAgent::HealthStatusImpl::HealthTask task2(m_mockPluginManager);
    task2.run();
    // Status Updated, Cached and file written due to cached cleared.
    ManagementAgent::HealthStatusImpl::HealthTask task3(m_mockPluginManager);
    task3.run();
}

TEST_F(HealthTaskTests, run_healthStatusMessageIsUpdatedWithCorrectUtmInformationWhenStatusFileFailsToWriteSuccessfully)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
            std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string expectedXmlGood = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="true" activeHeartbeatUtmId="some-random-utm-id-0"><item name="health" value="1" /><item name="admin" value="1" /><item name="service" value="1" ><detail name="Sophos MCS Client" value="0" /><detail name="Plugin One" value="0" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>)";
    std::string expectedXmlBad = R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="3" /><item name="admin" value="1" /><item name="service" value="3" ><detail name="Sophos MCS Client" value="0" /><detail name="Plugin One" value="1" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>)";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthGood;
    pluginHealthGood.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginHealthGood.healthValue = 0;
    pluginHealthGood.displayName = "Plugin One";
    pluginHealthGood.activeHeartbeat = true;
    pluginHealthGood.activeHeartbeatUtmId = "some-random-utm-id-0";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthBad = pluginHealthGood;
    pluginHealthBad.healthValue = 1;
    pluginHealthBad.activeHeartbeatUtmId = "";
    pluginHealthBad.activeHeartbeat = false;

    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_healthFilePath, _, m_tempDir, m_statusFileMode)).Times(1);
    EXPECT_CALL(*filesystemMock, isFile(m_healthFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(m_healthFilePath)).WillRepeatedly(Return("{\"health\":1,\"service\":1,\"threatService\":1,\"threat\":1}"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXmlBad, m_tempDir, m_statusFileMode)).Times(1).RetiresOnSaturation();

    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXmlBad, m_tempDir, m_statusFileMode)).
            WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST"))).RetiresOnSaturation();

    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXmlGood, m_tempDir, m_statusFileMode)).Times(1);
    EXPECT_CALL(
            *filesystemMock,
            isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
            .WillOnce(Return(false));
    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).Times(3).WillRepeatedly(Return(std::vector<std::string>{"pluginone"}));
    EXPECT_CALL(m_mockPluginManager, getHealthStatusForPlugin("pluginone", false)).Times(2).WillRepeatedly(Return(pluginHealthBad));
    EXPECT_CALL(m_mockPluginManager, getHealthStatusForPlugin("pluginone", false)).WillOnce(Return(pluginHealthGood)).RetiresOnSaturation();
    auto healthStatus = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    EXPECT_CALL(m_mockPluginManager, getSharedHealthStatusObj()).WillRepeatedly(Return(healthStatus));

    // Status Updated, Cached and file written
    ManagementAgent::HealthStatusImpl::HealthTask task1(m_mockPluginManager);
    task1.run();
    // Status Updated, Cached and file written results in ERROR
    ManagementAgent::HealthStatusImpl::HealthTask task2(m_mockPluginManager);
    task2.run();
    // Status Updated, Cached and file written due to cached cleared.
    ManagementAgent::HealthStatusImpl::HealthTask task3(m_mockPluginManager);
    task3.run();
}

TEST_F(HealthTaskTests, run_healthRemovesMissingPluginsIfNotFoundInRegistry)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
        std::make_unique<Tests::ScopedReplaceFileSystem>(
            std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthMissing;
    pluginHealthMissing.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginHealthMissing.healthValue = 2;
    pluginHealthMissing.displayName = "Plugin";
    pluginHealthMissing.activeHeartbeat = false;
    pluginHealthMissing.activeHeartbeatUtmId = "some-random-utm-id-0";

    // No bad plugins, since they've been removed, results in good health
    std::string expectedXml =
        R"(<?xml version="1.0" encoding="utf-8" ?><health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId=""><item name="health" value="1" /><item name="admin" value="1" /><item name="service" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threatService" value="1" ><detail name="Sophos MCS Client" value="0" /></item><item name="threat" value="1" /></health>)";

    EXPECT_CALL(*filesystemMock, isFile(m_healthFilePath)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(m_healthFilePath))
        .WillRepeatedly(Return("{\"health\":1,\"service\":1,\"threatService\":1,\"threat\":1}"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically(m_statusFilePath, expectedXml, m_tempDir, m_statusFileMode))
        .Times(1)
        .RetiresOnSaturation();

    EXPECT_CALL(
        *filesystemMock,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getThreatHealthJsonFilePath()))
        .WillOnce(Return(false));

    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).WillOnce(Return(std::vector<std::string>{ "plugin" }));
    EXPECT_CALL(m_mockPluginManager, getHealthStatusForPlugin("plugin", false)).WillOnce(Return(pluginHealthMissing));
    auto healthStatus = std::make_shared<ManagementAgent::HealthStatusImpl::HealthStatus>();
    EXPECT_CALL(m_mockPluginManager, checkIfSinglePluginInRegistry("plugin")).WillOnce(Return(false));
    EXPECT_CALL(m_mockPluginManager, removePlugin("plugin"));
    EXPECT_CALL(m_mockPluginManager, getSharedHealthStatusObj()).WillRepeatedly(Return(healthStatus));

    ManagementAgent::HealthStatusImpl::HealthTask task(m_mockPluginManager);
    task.run();
}
