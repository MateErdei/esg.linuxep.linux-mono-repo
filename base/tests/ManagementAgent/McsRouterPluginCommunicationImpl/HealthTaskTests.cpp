/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "MockPluginManager.h"

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <FileSystem/IFileSystemException.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/HealthTask.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

class HealthTaskTests : public testing::Test
{
protected:
    void SetUp() override
    {
        m_healthStatus = std::make_shared<ManagementAgent::McsRouterPluginCommunicationImpl::HealthStatus>();

        auto xml = m_healthStatus->generateHealthStatusXml();
        EXPECT_TRUE(xml.first);
    }
    void TearDown() override {}

    StrictMock<MockPluginManager> m_mockPluginManager;
    std::shared_ptr<ManagementAgent::McsRouterPluginCommunicationImpl::HealthStatus> m_healthStatus;

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(HealthTaskTests, run_healthStatusMessageIsUpdatedWhenDifferentFromCachedValue) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
        std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\"><item name=\"health\" value=\"1\" /><item name=\"service\" value=\"1\" ><detail name=\"Sophos MCS Client\" value=\"0\" /></item><item name=\"threatService\" value=\"1\" ><detail name=\"Sophos MCS Client\" value=\"0\" /></item><item name=\"threat\" value=\"1\" /><item name=\"admin\" value=\"1\" /></health>";

    std::string statusFilePAth = "/opt/sophos-spl/base/mcs/status/SHS_status.xml";
    EXPECT_CALL(*filesystemMock, writeFile(statusFilePAth,expectedXml)).Times(1);
    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).WillOnce(Return(std::vector<std::string>{}));

    ManagementAgent::McsRouterPluginCommunicationImpl::HealthTask task(m_mockPluginManager, m_healthStatus);
    task.run();
}

TEST_F(HealthTaskTests, run_healthStatusMessageIsNotUpdatedWhenStatusValueHasNotChanged) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
        std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string expectedXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\"><item name=\"health\" value=\"1\" /><item name=\"service\" value=\"1\" ><detail name=\"Sophos MCS Client\" value=\"0\" /></item><item name=\"threatService\" value=\"1\" ><detail name=\"Sophos MCS Client\" value=\"0\" /></item><item name=\"threat\" value=\"1\" /><item name=\"admin\" value=\"1\" /></health>";
    std::string statusFilePAth = "/opt/sophos-spl/base/mcs/status/SHS_status.xml";
    EXPECT_CALL(*filesystemMock, writeFile(statusFilePAth,expectedXml)).Times(1);
    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).Times(2).WillRepeatedly(Return(std::vector<std::string>{}));

    ManagementAgent::McsRouterPluginCommunicationImpl::HealthTask task1(m_mockPluginManager, m_healthStatus);
    task1.run();

    ManagementAgent::McsRouterPluginCommunicationImpl::HealthTask task2(m_mockPluginManager, m_healthStatus);
    task2.run();
}

TEST_F(HealthTaskTests, run_healthStatusMessageIsUpdatedWhenStatusFileFailsToWriteSuccessfully) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::unique_ptr<Tests::ScopedReplaceFileSystem> scopedReplaceFileSystem =
        std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    std::string expectedXmlGood = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\"><item name=\"health\" value=\"1\" /><item name=\"service\" value=\"1\" ><detail name=\"Sophos MCS Client\" value=\"0\" /><detail name=\"Plugin One\" value=\"0\" /></item><item name=\"threatService\" value=\"1\" ><detail name=\"Sophos MCS Client\" value=\"0\" /></item><item name=\"threat\" value=\"1\" /><item name=\"admin\" value=\"1\" /></health>";
    std::string expectedXmlBad = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><health version=\"3.0.0\" activeHeartbeat=\"false\" activeHeartbeatUtmId=\"\"><item name=\"health\" value=\"3\" /><item name=\"service\" value=\"3\" ><detail name=\"Sophos MCS Client\" value=\"0\" /><detail name=\"Plugin One\" value=\"1\" /></item><item name=\"threatService\" value=\"1\" ><detail name=\"Sophos MCS Client\" value=\"0\" /></item><item name=\"threat\" value=\"1\" /><item name=\"admin\" value=\"1\" /></health>";

    std::string statusFilePAth = "/opt/sophos-spl/base/mcs/status/SHS_status.xml";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthGood;
    pluginHealthGood.healthType = ManagementAgent::PluginCommunication::HealthType::SERVICE;
    pluginHealthGood.healthValue = 0;
    pluginHealthGood.displayName = "Plugin One";

    ManagementAgent::PluginCommunication::PluginHealthStatus pluginHealthBad = pluginHealthGood;
    pluginHealthBad.healthValue = 1;

    EXPECT_CALL(*filesystemMock, writeFile(statusFilePAth,expectedXmlBad)).Times(1).RetiresOnSaturation();

    EXPECT_CALL(*filesystemMock, writeFile(statusFilePAth,expectedXmlBad)).
        WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST"))).RetiresOnSaturation();

    EXPECT_CALL(*filesystemMock, writeFile(statusFilePAth,expectedXmlGood)).Times(1);

    EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).Times(3).WillRepeatedly(Return(std::vector<std::string>{"pluginone"}));
    EXPECT_CALL(m_mockPluginManager, getHealthStatusForPlugin("pluginone")).Times(2).WillRepeatedly(Return(pluginHealthBad));
    EXPECT_CALL(m_mockPluginManager, getHealthStatusForPlugin("pluginone")).WillOnce(Return(pluginHealthGood)).RetiresOnSaturation();

    // Status Updated, Cached and file written
    ManagementAgent::McsRouterPluginCommunicationImpl::HealthTask task1(m_mockPluginManager, m_healthStatus);
    task1.run();
    // Status Updated, Cached and file written results in ERROR
    ManagementAgent::McsRouterPluginCommunicationImpl::HealthTask task2(m_mockPluginManager, m_healthStatus);
    task2.run();
    // Status Updated, Cached and file written due to cached cleared.
    ManagementAgent::McsRouterPluginCommunicationImpl::HealthTask task3(m_mockPluginManager, m_healthStatus);
    task3.run();
}