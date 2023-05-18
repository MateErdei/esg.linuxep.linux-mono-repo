// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "MockEventWriterWorker.h"
#include "MockSubscriberLib.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockApiBaseServices.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <SubscriberLib/Subscriber.h>
#include <gtest/gtest.h>
#include <modules/Heartbeat/Heartbeat.h>
#include <modules/Heartbeat/MockHeartbeatPinger.h>
#include <modules/pluginimpl/Logger.h>
#include <pluginimpl/PluginAdapter.h>
#include <pluginimpl/TaskQueue.h>

#include <atomic>
#include <future>
#include <utility>

using namespace Common::FileSystem;

class TestablePluginAdapter : public Plugin::PluginAdapter
{
public:
    TestablePluginAdapter(
        const std::shared_ptr<Plugin::TaskQueue>& queueTask,
        std::unique_ptr<SubscriberLib::ISubscriber> subscriber,
        std::unique_ptr<EventWriterLib::IEventWriterWorker> eventWriter,
        std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger,
        const std::shared_ptr<Heartbeat::IHeartbeat>& heartbeat
        ) :
        Plugin::PluginAdapter(
            queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi>(std::make_unique<StrictMock<MockApiBaseServices>>()),
            std::make_shared<Plugin::PluginCallback>(queueTask, heartbeat),
            std::move(subscriber),
            std::move(eventWriter),
            std::move(heartbeatPinger))
    {
    }
};

class PluginAdapterTests : public LogOffInitializedTests
{
};

TEST_F(PluginAdapterTests, PluginAdapterRestartsSubscriberOrWriterIfTheyStop)
{
    std::atomic_int subscriberRunningStatusCall = 0;
    auto countSubscriberStatusCalls = [&]()
    {
        subscriberRunningStatusCall++;
        return false;
    };

    std::atomic_int writerRunningStatusCall = 0;
    auto countWriterStatusCalls = [&]()
    {
      writerRunningStatusCall++;
      return false;
    };

    // Mock Subscriber
    MockSubscriberLib* mockSubscriber = new StrictMock<MockSubscriberLib>();
    EXPECT_CALL(*mockSubscriber, start).Times(1); // Plugin starting up subscriber
    EXPECT_CALL(*mockSubscriber, stop).Times(1);  // Plugin stopping subscriber on stop task
    EXPECT_CALL(*mockSubscriber, getRunningStatus).WillRepeatedly(Invoke(countSubscriberStatusCalls));
    EXPECT_CALL(*mockSubscriber, restart).Times(1);
    std::unique_ptr<SubscriberLib::ISubscriber> mockSubscriberPtr(mockSubscriber);

    // Mock EventWriterWorker
    MockEventWriterWorker* mockEventWriterWorker = new StrictMock<MockEventWriterWorker>();
    EXPECT_CALL(*mockEventWriterWorker, start).Times(1); // Plugin starting up Event Writer Worker
    EXPECT_CALL(*mockEventWriterWorker, stop).Times(1);  // Plugin stopping Event Writer Worker on stop task
    EXPECT_CALL(*mockEventWriterWorker, getRunningStatus).WillRepeatedly(Invoke(countWriterStatusCalls));
    EXPECT_CALL(*mockEventWriterWorker, restart).Times(1);
    std::unique_ptr<EventWriterLib::IEventWriterWorker> mockEventWriterWorkerPtr(mockEventWriterWorker);

    // Queue
    auto queueTask = std::make_shared<Plugin::TaskQueue>();

    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();

    TestablePluginAdapter pluginAdapter(
            queueTask,
            std::move(mockSubscriberPtr),
            std::move(mockEventWriterWorkerPtr),
            heartbeat->getPingHandleForId("PluginAdapterThread"),
            heartbeat);

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &pluginAdapter);
    while (subscriberRunningStatusCall == 0)
    {
        usleep(100);
    }
    while (writerRunningStatusCall == 0)
    {
        usleep(100);
    }
    queueTask->pushStop();
    mainLoopFuture.get();
}

TEST_F(PluginAdapterTests, PluginAdapterMainLoopThrowsIfSocketDirDoesNotExist)
{
    MockSubscriberLib* mockSubscriber = new StrictMock<MockSubscriberLib>();
    EXPECT_CALL(*mockSubscriber, start).WillOnce(Throw(std::runtime_error("Exception on start")));
    std::unique_ptr<SubscriberLib::ISubscriber> mockSubscriberPtr(mockSubscriber);

    // Mock EventWriterWorker
    MockEventWriterWorker* mockEventWriterWorker = new StrictMock<MockEventWriterWorker>();
    EXPECT_CALL(*mockEventWriterWorker, start).Times(1); // Plugin starting up subscriber
    std::unique_ptr<EventWriterLib::IEventWriterWorker> mockEventWriterWorkerPtr(mockEventWriterWorker);

    auto queueTask = std::make_shared<Plugin::TaskQueue>();
    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();
    TestablePluginAdapter pluginAdapter(
            queueTask,
            std::move(mockSubscriberPtr),
            std::move(mockEventWriterWorkerPtr),
            heartbeat->getPingHandleForId("PluginAdapterThread"),
            heartbeat);

    // Example exception on start: If the socket dir does not exist then the whole plugin will exit.
    // If that dir is missing then the installation is unusable and this plugin shouldn't try and fix it.
    // So throw and let WD start us up again if needed.
    EXPECT_THROW(pluginAdapter.mainLoop(), std::runtime_error);
}

TEST_F(PluginAdapterTests, testMainloopPingsHeartbeatRepeatedly)
{
    // Mock Subscriber
    MockSubscriberLib* mockSubscriber = new NiceMock<MockSubscriberLib>();
    std::unique_ptr<SubscriberLib::ISubscriber> mockSubscriberPtr(mockSubscriber);

    // Mock EventWriterWorker
    MockEventWriterWorker* mockEventWriterWorker = new NiceMock<MockEventWriterWorker>();
    std::unique_ptr<EventWriterLib::IEventWriterWorker> mockEventWriterWorkerPtr(mockEventWriterWorker);

    // Queue
    auto queueTask = std::make_shared<Plugin::TaskQueue>();

    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();
    auto mockHeartbeatPinger = std::make_shared<StrictMock<Heartbeat::MockHeartbeatPinger>>();
    EXPECT_CALL(*mockHeartbeatPinger, ping).Times(AtLeast(2));
    TestablePluginAdapter pluginAdapter(
            queueTask,
            std::move(mockSubscriberPtr),
            std::move(mockEventWriterWorkerPtr),
            mockHeartbeatPinger,
            heartbeat);

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &pluginAdapter);
    //Mainloop blocks for 5s waiting for tasks each time.
    sleep(6);
    queueTask->pushStop();
    mainLoopFuture.get();
}