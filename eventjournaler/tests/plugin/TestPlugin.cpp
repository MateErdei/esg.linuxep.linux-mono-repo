// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "MockEventWriterWorker.h"
#include "MockSubscriberLib.h"

#include "SubscriberLib/Subscriber.h"
#include "Heartbeat/Heartbeat.h"
#include "Heartbeat/MockHeartbeatPinger.h"
#include "pluginimpl/PluginAdapter.h"
#include "pluginimpl/TaskQueue.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockApiBaseServices.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockApiBaseServices.h"
#endif

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <gtest/gtest.h>

#include <atomic>
#include <future>
#include <utility>


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

    void setQueueTimeout(std::chrono::milliseconds timeout)
    {
        QUEUE_TIMEOUT = timeout;
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
    auto mockSubscriberPtr = std::make_unique<StrictMock<MockSubscriberLib>>();
    EXPECT_CALL(*mockSubscriberPtr, start).Times(1); // Plugin starting up subscriber
    EXPECT_CALL(*mockSubscriberPtr, stop).Times(1);  // Plugin stopping subscriber on stop task
    EXPECT_CALL(*mockSubscriberPtr, getRunningStatus).WillRepeatedly(Invoke(countSubscriberStatusCalls));
    EXPECT_CALL(*mockSubscriberPtr, restart).Times(1);

    // Mock EventWriterWorker
    auto mockEventWriterWorkerPtr = std::make_unique<StrictMock<MockEventWriterWorker>>();
    EXPECT_CALL(*mockEventWriterWorkerPtr, start).Times(1); // Plugin starting up Event Writer Worker
    EXPECT_CALL(*mockEventWriterWorkerPtr, beginStop).Times(1);  // Plugin stopping Event Writer Worker on stop task
    EXPECT_CALL(*mockEventWriterWorkerPtr, stop).Times(1);  // Plugin stopping Event Writer Worker on stop task
    EXPECT_CALL(*mockEventWriterWorkerPtr, getRunningStatus).WillRepeatedly(Invoke(countWriterStatusCalls));
    EXPECT_CALL(*mockEventWriterWorkerPtr, restart).Times(1);

    // Queue
    auto queueTask = std::make_shared<Plugin::TaskQueue>();

    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();

    TestablePluginAdapter pluginAdapter(
            queueTask,
            std::move(mockSubscriberPtr),
            std::move(mockEventWriterWorkerPtr),
            heartbeat->getPingHandleForId("PluginAdapterThread"),
            heartbeat);

    pluginAdapter.setQueueTimeout(std::chrono::milliseconds{30});

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &pluginAdapter);
    while (subscriberRunningStatusCall == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    while (writerRunningStatusCall == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
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
    auto mockSubscriberPtr = std::make_unique<NiceMock<MockSubscriberLib>>();

    // Mock EventWriterWorker
    auto mockEventWriterWorkerPtr = std::make_unique<NiceMock<MockEventWriterWorker>>();

    // Queue
    auto queueTask = std::make_shared<Plugin::TaskQueue>();

    auto heartbeat = std::make_shared<Heartbeat::Heartbeat>();
    auto mockHeartbeatPinger = std::make_shared<StrictMock<Heartbeat::MockHeartbeatPinger>>();
    int callCounter = 0;
    EXPECT_CALL(*mockHeartbeatPinger, ping).Times(AtLeast(2)).WillRepeatedly(
            InvokeWithoutArgs([&callCounter](){
                              callCounter++;
        }));
    TestablePluginAdapter pluginAdapter(
            queueTask,
            std::move(mockSubscriberPtr),
            std::move(mockEventWriterWorkerPtr),
            mockHeartbeatPinger,
            heartbeat);

    //Mainloop blocks for 5s waiting for tasks each time by default, but we want this test to be quicker
    pluginAdapter.setQueueTimeout(std::chrono::milliseconds(10));

    auto mainLoopFuture = std::async(std::launch::async, &TestablePluginAdapter::mainLoop, &pluginAdapter);
    while (callCounter < 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    queueTask->pushStop();
    mainLoopFuture.get();
}