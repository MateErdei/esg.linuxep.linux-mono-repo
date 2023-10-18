// Copyright 2021-2023 Sophos Limited. All rights reserved.

#ifdef SPL_BAZEL
#include "JournalerCommon/Event.h"
#include "SubscriberLib/EventQueuePusher.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#else
#include "modules/JournalerCommon/Event.h"
#include "modules/SubscriberLib/EventQueuePusher.h"
#include "Common/Helpers/LogInitializedTests.h"
#endif
#include "Common/FileSystem/IFileSystem.h"

#include <gtest/gtest.h>

#include <future>
#include <thread>
#include <utility>


class TestEventQueue : public LogOffInitializedTests{};

using namespace EventQueueLib;

namespace
{
    std::queue<JournalerCommon::Event> createQueueOfSize(int queueSize)
    {
        auto queue =  std::queue<JournalerCommon::Event>{};
        for (int i = 0; i < queueSize; i++)
        {
            queue.push(JournalerCommon::Event());
        }
        EXPECT_EQ(queue.size(), queueSize);
        return queue;
    }
}

struct TestableEventQueue : public EventQueue
{
    explicit TestableEventQueue(int maxSize)
        : EventQueue::EventQueue(maxSize)
    {}

    bool isQueueFull() // NOLINT
    {
        return EventQueue::isQueueFull();
    }

    bool isQueueEmpty() // NOLINT
    {
        return EventQueue::isQueueEmpty();
    }

    void setQueue(std::queue<JournalerCommon::Event> newQueue)
    {
        m_queue = std::move(newQueue);
    }

    std::queue<JournalerCommon::Event> getQueue()
    {
        return m_queue;
    }
};

TEST_F(TestEventQueue, testEventQueueIsFullReturnsTrueWhenQueueIsFull)
{
    TestableEventQueue eventQueueWithMaxSize1(1);
    auto queueWithNoItems =  createQueueOfSize(0);
    auto queueWithOneItem =  createQueueOfSize(1);
    auto queueWithTwoItem =  createQueueOfSize(2);
    eventQueueWithMaxSize1.setQueue(queueWithNoItems);
    EXPECT_FALSE(eventQueueWithMaxSize1.isQueueFull());
    eventQueueWithMaxSize1.setQueue(queueWithOneItem);
    EXPECT_TRUE(eventQueueWithMaxSize1.isQueueFull());
    eventQueueWithMaxSize1.setQueue(queueWithTwoItem);
    EXPECT_TRUE(eventQueueWithMaxSize1.isQueueFull());

    TestableEventQueue eventQueueWithMaxSize5(5);
    auto queueWith5Items =  createQueueOfSize(5);
    auto queueWith3Items =  createQueueOfSize(3);
    eventQueueWithMaxSize5.setQueue(queueWithNoItems);
    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueFull());
    eventQueueWithMaxSize5.setQueue(queueWith5Items);
    EXPECT_TRUE(eventQueueWithMaxSize5.isQueueFull());
    eventQueueWithMaxSize5.setQueue(queueWith3Items);
    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueFull());
}

TEST_F(TestEventQueue, testEventQueueIsEmptyReturnsTrueWhenQueueIsEmpty)
{
    TestableEventQueue eventQueueWithMaxSize5(5);
    auto queueWith0Items =  createQueueOfSize(0);
    auto queueWith1Items =  createQueueOfSize(1);
    auto queueWith5Items =  createQueueOfSize(5);
    auto queueWith3Items =  createQueueOfSize(3);

    eventQueueWithMaxSize5.setQueue(queueWith0Items);
    EXPECT_TRUE(eventQueueWithMaxSize5.isQueueEmpty());
    eventQueueWithMaxSize5.setQueue(queueWith1Items);
    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueEmpty());
    eventQueueWithMaxSize5.setQueue(queueWith5Items);
    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueEmpty());
    eventQueueWithMaxSize5.setQueue(queueWith3Items);
    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueEmpty());
}


TEST_F(TestEventQueue, testEventQueuePushReturnsTrueWhenPushingToQueueThatIsNotFull)
{
    TestableEventQueue eventQueueWithMaxSize5(5);
    JournalerCommon::Event data {JournalerCommon::EventType::THREAT_EVENT, "fake data"};
    EXPECT_TRUE(eventQueueWithMaxSize5.push(data));
    auto queue = eventQueueWithMaxSize5.getQueue();
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.front().data, data.data);
}

TEST_F(TestEventQueue, testEventQueuePushReturnsFalseWhenPushingToQueueThatIsFull)
{
    TestableEventQueue eventQueueWithMaxSize2(2);
    auto mockedQueue = std::queue<JournalerCommon::Event>();
    JournalerCommon::Event data1 {JournalerCommon::EventType::THREAT_EVENT, "fake data one"};
    JournalerCommon::Event data2 {JournalerCommon::EventType::THREAT_EVENT, "fake data two"};
    JournalerCommon::Event data3 {JournalerCommon::EventType::THREAT_EVENT, "fake data three"};
    mockedQueue.push(data1);
    mockedQueue.push(data2);
    eventQueueWithMaxSize2.setQueue(mockedQueue);

    EXPECT_FALSE(eventQueueWithMaxSize2.push(data3));
    auto queue = eventQueueWithMaxSize2.getQueue();
    EXPECT_NE(&mockedQueue, &queue);
    EXPECT_EQ(queue.front().data, data1.data);
    EXPECT_EQ(queue.back().data, data2.data);
}

TEST_F(TestEventQueue, testEventQueuePopBlocksForTimeoutBeforeReturningEmptyOptionalWhenNoDataToPop)
{
    TestableEventQueue eventQueue(2);
    constexpr auto timeout = 50;

    auto before = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    auto emptyOptionalData = eventQueue.pop(timeout);
    auto after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    EXPECT_FALSE(emptyOptionalData.has_value());
    auto duration = after.count() - before.count();
    EXPECT_GE(duration, timeout);
    EXPECT_NEAR(duration, timeout, 10);
}

TEST_F(TestEventQueue, testEventQueuePopReturnsValueImmediatelyWhenThereIsDataToPop)
{
    TestableEventQueue eventQueueWithMaxSize2(2);
    auto mockedQueue = std::queue<JournalerCommon::Event>();
    JournalerCommon::Event expectedData1 {JournalerCommon::EventType::THREAT_EVENT, "fake data one"};
    JournalerCommon::Event expectedData2 {JournalerCommon::EventType::THREAT_EVENT, "fake data two"};
    mockedQueue.push(expectedData1);
    mockedQueue.push(expectedData2);
    eventQueueWithMaxSize2.setQueue(mockedQueue);

    constexpr auto delay = 50;

    std::chrono::milliseconds before1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::optional<JournalerCommon::Event> data1 = eventQueueWithMaxSize2.pop(delay*10);
    std::chrono::milliseconds after1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    std::chrono::milliseconds before2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::optional<JournalerCommon::Event> data2 = eventQueueWithMaxSize2.pop(delay*10);
    std::chrono::milliseconds after2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    EXPECT_EQ(data1->data, expectedData1.data);
    EXPECT_EQ(data2->data, expectedData2.data);
    EXPECT_LE(after1.count() - before1.count(), 10);
    EXPECT_LE(after2.count() - before2.count(), 10);
}

TEST_F(TestEventQueue, testEventQueuePopBlocksDuringTimeoutBeforeUnblockingAndReturningValueWhenDataIsPushed)
{
    TestableEventQueue eventQueueWithMaxSize2(2);
    JournalerCommon::Event expectedData {JournalerCommon::EventType::THREAT_EVENT, "fake data"};

    constexpr auto delay = 50;

    auto blockWhileWaitingForData = std::async(std::launch::async,
            [&eventQueueWithMaxSize2, &expectedData]
        {
            std::chrono::milliseconds before = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
            std::optional<JournalerCommon::Event> data = eventQueueWithMaxSize2.pop(delay*10);
            std::chrono::milliseconds after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
            EXPECT_TRUE(data.has_value());
            EXPECT_EQ(data->data, expectedData.data);
            return after.count() - before.count();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    eventQueueWithMaxSize2.push(expectedData);
    auto duration = blockWhileWaitingForData.get();
    EXPECT_GE(duration, delay);
    EXPECT_NEAR(duration, delay, 15);
}

TEST_F(TestEventQueue, testPushedDataIsCorrectlyQueuedAndReturnedWhenPopped)
{
    auto eventQueuePtr = std::make_shared<EventQueueLib::EventQueue>(3);
    auto pusher = std::make_unique<SubscriberLib::EventQueuePusher>(eventQueuePtr);
    auto popper = eventQueuePtr;

    JournalerCommon::Event data1 {JournalerCommon::EventType::THREAT_EVENT, "fake data one"};
    JournalerCommon::Event data2 {JournalerCommon::EventType::THREAT_EVENT, "fake data two"};
    JournalerCommon::Event data3 {JournalerCommon::EventType::THREAT_EVENT, "fake data three"};

    pusher->handleEvent(data1);
    pusher->handleEvent(data2);
    pusher->handleEvent(data3);

    ASSERT_EQ(data1.data, popper->pop(10)->data);
    ASSERT_EQ(data2.data, popper->pop(10)->data);
    ASSERT_EQ(data3.data, popper->pop(10)->data);
}

TEST_F(TestEventQueue, stopPreventsPop)
{
    auto eventQueue = std::make_shared<EventQueueLib::EventQueue>(3);
    JournalerCommon::Event data1 {JournalerCommon::EventType::THREAT_EVENT, "fake data one"};
    eventQueue->push(data1);
    eventQueue->stop();
    auto event = eventQueue->pop(10);
    EXPECT_FALSE(event.has_value());
}

TEST_F(TestEventQueue, stopWakesUpLongPop)
{
    auto eventQueue = std::make_shared<EventQueueLib::EventQueue>(3);
    constexpr auto delay = 5;

    auto blockWhileWaitingForData = std::async(std::launch::async,
       [&eventQueue]
       {
           std::chrono::milliseconds before = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
           std::optional<JournalerCommon::Event> data = eventQueue->pop(delay*10);
           std::chrono::milliseconds after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
           EXPECT_FALSE(data.has_value());
           return after.count() - before.count();
       });

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    eventQueue->stop();
    auto duration = blockWhileWaitingForData.get();
    EXPECT_GE(duration, delay);
    EXPECT_LE(duration, delay*3);
}

TEST_F(TestEventQueue,restartAllowsPop)
{
    auto eventQueue = std::make_shared<EventQueueLib::EventQueue>(3);
    JournalerCommon::Event data1 {JournalerCommon::EventType::THREAT_EVENT, "fake data one"};
    eventQueue->push(data1);
    eventQueue->stop();
    {
        auto event = eventQueue->pop(100);
        EXPECT_FALSE(event.has_value());
    }
    eventQueue->restart();
    {
        auto event = eventQueue->pop(100);
        EXPECT_TRUE(event.has_value());
    }
}
