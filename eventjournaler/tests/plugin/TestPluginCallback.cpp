/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/pluginimpl/PluginCallback.h>
#include <modules/pluginimpl/QueueTask.h>
#include <modules/Heartbeat/ThreadIdConsts.h>
#include <modules/pluginimpl/ApplicationPaths.h>

#include <gtest/gtest.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <modules/Heartbeat/Heartbeat.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <modules/Heartbeat/MockHeartbeat.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>


class PluginCallbackTests : public LogOffInitializedTests
{
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }

};

TEST_F(PluginCallbackTests, testPluginAdapterRegistersExpectedIDs)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();
    auto sharedPluginCallBack = std::make_shared<Plugin::PluginCallback>(queueTask, heartbeat);

    std::map<std::string, bool> map = heartbeat->getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 3);
    EXPECT_EQ(map.count(Heartbeat::getWriterThreadId()), true);
    EXPECT_EQ(map.count(Heartbeat::getSubscriberThreadId()), true);
    EXPECT_EQ(map.count(Heartbeat::getPluginAdapterThreadId()), true);
}

TEST_F(PluginCallbackTests, testGetHealthReturns0WhenAllFactorsHealthy)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    EXPECT_CALL(*mockFileSystem, exists(Plugin::getSubscriberSocketPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockHeartbeat, getNumDroppedEventsInLast24h()).WillOnce(Return(0));
    std::map<std::string, bool> returnMap = {{"Writer", true}, {"Subscriber", true}, {"PluginAdapter", true}};
    EXPECT_CALL(*mockHeartbeat, getMapOfIdsAgainstIsAlive()).WillOnce(Return(returnMap));
    ASSERT_EQ(sharedPluginCallBack.getHealth(), "{'Health': 0}");

    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    auto telemetryJson = telemetry.serialise();
    EXPECT_TRUE(telemetryJson.find("acceptable-daily-dropped-events-exceeded\":false") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("event-subscriber-socket-missing\":false") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("thread-health\":{\"PluginAdapter\":true,\"Subscriber\":true,\"Writer\":true}") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("health\":0") != std::string::npos);
}

TEST_F(PluginCallbackTests, testGetHealthReturns1WhenExceedingMaxAcceptableDroppedEvents)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    EXPECT_CALL(*mockFileSystem, exists(Plugin::getSubscriberSocketPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockHeartbeat, getNumDroppedEventsInLast24h()).WillOnce(Return(6));
    std::map<std::string, bool> returnMap = {{"Writer", true}, {"Subscriber", true}, {"PluginAdapter", true}};
    EXPECT_CALL(*mockHeartbeat, getMapOfIdsAgainstIsAlive()).WillOnce(Return(returnMap));
    ASSERT_EQ(sharedPluginCallBack.getHealth(), "{'Health': 1}");

    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    auto telemetryJson = telemetry.serialise();
    EXPECT_TRUE(telemetryJson.find("acceptable-daily-dropped-events-exceeded\":true") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("event-subscriber-socket-missing\":false") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("thread-health\":{\"PluginAdapter\":true,\"Subscriber\":true,\"Writer\":true}") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("health\":1") != std::string::npos);

}

TEST_F(PluginCallbackTests, testGetHealthReturns1WhenSocketMissing)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    EXPECT_CALL(*mockFileSystem, exists(Plugin::getSubscriberSocketPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockHeartbeat, getNumDroppedEventsInLast24h()).WillOnce(Return(0));
    std::map<std::string, bool> returnMap = {{"Writer", true}, {"Subscriber", true}, {"PluginAdapter", true}};
    EXPECT_CALL(*mockHeartbeat, getMapOfIdsAgainstIsAlive()).WillOnce(Return(returnMap));
    ASSERT_EQ(sharedPluginCallBack.getHealth(), "{'Health': 1}");

    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    auto telemetryJson = telemetry.serialise();
    EXPECT_TRUE(telemetryJson.find("acceptable-daily-dropped-events-exceeded\":false") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("event-subscriber-socket-missing\":true") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("thread-health\":{\"PluginAdapter\":true,\"Subscriber\":true,\"Writer\":true}") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("health\":1") != std::string::npos);

}

TEST_F(PluginCallbackTests, testGetHealthReturns1WhenThreadsDead)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat);

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    EXPECT_CALL(*mockFileSystem, exists(Plugin::getSubscriberSocketPath())).WillOnce(Return(true));
    std::map<std::string, bool> returnMap = {{"Writer", false}, {"Subscriber", true}, {"PluginAdapter", true}};
    EXPECT_CALL(*mockHeartbeat, getMapOfIdsAgainstIsAlive()).WillOnce(Return(returnMap));
    EXPECT_CALL(*mockHeartbeat, getNumDroppedEventsInLast24h()).WillOnce(Return(0));
    ASSERT_EQ(sharedPluginCallBack.getHealth(), "{'Health': 1}");

    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    auto telemetryJson = telemetry.serialise();
    EXPECT_TRUE(telemetryJson.find("acceptable-daily-dropped-events-exceeded\":false") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("event-subscriber-socket-missing\":false") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("thread-health\":{\"PluginAdapter\":true,\"Subscriber\":true,\"Writer\":false}") != std::string::npos);
    EXPECT_TRUE(telemetryJson.find("health\":1") != std::string::npos);

}

class PluginCallbackWithMockedHealthInner : public  Plugin::PluginCallback {
public:
    MOCK_METHOD0(getHealthInner, uint());

    PluginCallbackWithMockedHealthInner(std::shared_ptr<Plugin::QueueTask> task,
                                        std::shared_ptr<StrictMock<Heartbeat::MockHeartbeat>> heartbeat)
            : Plugin::PluginCallback(task, heartbeat)
            {};
};

TEST_F(PluginCallbackTests, testGetTelemetryCallsGetHealthInner)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = PluginCallbackWithMockedHealthInner(queueTask, mockHeartbeat);

    EXPECT_CALL(sharedPluginCallBack, getHealthInner()).Times(1);
    sharedPluginCallBack.getTelemetry();
}