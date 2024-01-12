// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "Heartbeat/Heartbeat.h"
#include "Heartbeat/MockHeartbeat.h"
#include "Heartbeat/ThreadIdConsts.h"
#include "pluginimpl/ApplicationPaths.h"
#include "pluginimpl/PluginCallback.h"
#include "pluginimpl/TaskQueue.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#endif

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <gtest/gtest.h>

class PluginCallbackTests : public LogOffInitializedTests
{
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }

};

TEST_F(PluginCallbackTests, testPluginAdapterRegistersExpectedIDs)
{
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();
    auto sharedPluginCallBack = std::make_shared<Plugin::PluginCallback>(queueTask, heartbeat);

    std::map<std::string, bool> map = heartbeat->getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 3);
    EXPECT_EQ(map.count(Heartbeat::WriterThreadId), true);
    EXPECT_EQ(map.count(Heartbeat::SubscriberThreadId), true);
    EXPECT_EQ(map.count(Heartbeat::PluginAdapterThreadId), true);
}

TEST_F(PluginCallbackTests, testGetHealthReturns0WhenAllFactorsHealthy)
{
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat, 0);

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
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat, 0);

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
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat, 0);

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
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = Plugin::PluginCallback(queueTask, mockHeartbeat, 0);

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
    MOCK_METHOD0(getHealthInner, Plugin::Health());

    PluginCallbackWithMockedHealthInner(std::shared_ptr<Plugin::TaskQueue> task,
                                        std::shared_ptr<StrictMock<Heartbeat::MockHeartbeat>> heartbeat)
            : Plugin::PluginCallback(task, heartbeat)
            {};
};

TEST_F(PluginCallbackTests, testGetTelemetryCallsGetHealthInner)
{
    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto mockHeartbeat = std::make_shared<StrictMock<Heartbeat::MockHeartbeat>>();
    EXPECT_CALL(*mockHeartbeat, registerIds(std::vector<std::string>{ "Writer", "Subscriber", "PluginAdapter" }));
    auto sharedPluginCallBack = PluginCallbackWithMockedHealthInner(queueTask, mockHeartbeat);

    EXPECT_CALL(sharedPluginCallBack, getHealthInner()).Times(1);
    sharedPluginCallBack.getTelemetry();
}