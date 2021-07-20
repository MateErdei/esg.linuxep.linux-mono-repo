/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <gtest/gtest.h>
#include <Common/Helpers/LogInitializedTests.h>

#include <modules/SubscriberLib/EventQueuePusher.h>
#include <modules/EventWriterLib/EventQueuePopper.h>

#include <future>


class TestEventQueue : public LogOffInitializedTests{};

using namespace EventQueueLib;

namespace
{
    std::queue<Common::ZeroMQWrapper::data_t> createQueueOfSize(int queueSize)
    {
        auto queue =  std::queue<Common::ZeroMQWrapper::data_t>{};
        for (int i = 0; i < queueSize; i++)
        {
            queue.push(Common::ZeroMQWrapper::data_t());
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

    bool isQueueFull()
    {
        return EventQueue::isQueueFull();
    }

    bool isQueueEmpty()
    {
        return EventQueue::isQueueEmpty();
    }

    void setQueue(std::queue<Common::ZeroMQWrapper::data_t> newQueue)
    {
        m_queue = newQueue;
    }

    std::queue<Common::ZeroMQWrapper::data_t> getQueue()
    {
        return m_queue;
    }
};

TEST_F(TestEventQueue, testEventQueueIsFullReturnsTrueWhenQueueIsFull) // NOLINT
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

TEST_F(TestEventQueue, testEventQueueIsEmptyReturnsTrueWhenQueueIsEmpty) // NOLINT
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


TEST_F(TestEventQueue, testEventQueuePushReturnsTrueWhenPushingToQueueThatIsNotFull) // NOLINT
{
    TestableEventQueue eventQueueWithMaxSize5(5);
    auto data = Common::ZeroMQWrapper::data_t({"one", "two", "three"});
    EXPECT_TRUE(eventQueueWithMaxSize5.push(data));
    auto queue = eventQueueWithMaxSize5.getQueue();
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.front(), data);
}

TEST_F(TestEventQueue, testEventQueuePushReturnsFalseWhenPushingToQueueThatIsFull) // NOLINT
{
    TestableEventQueue eventQueueWithMaxSize2(2);
    auto mockedQueue = std::queue<Common::ZeroMQWrapper::data_t>();
    auto data1 = Common::ZeroMQWrapper::data_t({"one", "two", "three"});
    auto data2 = Common::ZeroMQWrapper::data_t({"four", "five", "six"});
    auto data3 = Common::ZeroMQWrapper::data_t({"seven", "eight", "nine"});
    mockedQueue.push(data1);
    mockedQueue.push(data2);
    eventQueueWithMaxSize2.setQueue(mockedQueue);

    EXPECT_FALSE(eventQueueWithMaxSize2.push(data3));
    auto queue = eventQueueWithMaxSize2.getQueue();
    EXPECT_EQ(mockedQueue, queue);
    EXPECT_EQ(queue.front(), data1);
    EXPECT_EQ(queue.back(), data2);
}

TEST_F(TestEventQueue, testEventQueuePopBlocksForTimeoutBeforeReturningEmptyOptionalWhenNoDataToPop) // NOLINT
{
    TestableEventQueue eventQueueWithMaxSize2(2);
    int timeout = 100;

    std::chrono::milliseconds before = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::optional<Common::ZeroMQWrapper::data_t> emptyOptionalData = eventQueueWithMaxSize2.pop(timeout);
    std::chrono::milliseconds after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    EXPECT_FALSE(emptyOptionalData.has_value());
    int duration = after.count() - before.count();
    EXPECT_GE(duration, timeout);
    EXPECT_NEAR(duration, timeout, 10);
}

TEST_F(TestEventQueue, testEventQueuePopReturnsValueImmediatelyWhenThereIsDataToPop) // NOLINT
{
    TestableEventQueue eventQueueWithMaxSize2(2);
    auto mockedQueue = std::queue<Common::ZeroMQWrapper::data_t>();
    auto expectedData1 = Common::ZeroMQWrapper::data_t({"one", "two", "three"});
    auto expectedData2 = Common::ZeroMQWrapper::data_t({"four", "five", "six"});
    mockedQueue.push(expectedData1);
    mockedQueue.push(expectedData2);
    eventQueueWithMaxSize2.setQueue(mockedQueue);


    std::chrono::milliseconds before1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::optional<Common::ZeroMQWrapper::data_t> data1 = eventQueueWithMaxSize2.pop(1000);
    std::chrono::milliseconds after1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    std::chrono::milliseconds before2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::optional<Common::ZeroMQWrapper::data_t> data2 = eventQueueWithMaxSize2.pop(1000);
    std::chrono::milliseconds after2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    EXPECT_EQ(data1, expectedData1);
    EXPECT_EQ(data2, expectedData2);
    EXPECT_NEAR(after1.count() - before1.count(), 0, 10);
    EXPECT_NEAR(after2.count() - before2.count(), 0, 10);
}

TEST_F(TestEventQueue, testEventQueuePopBlocksDuringTimeoutBeforeUnblockingAndReturningValueWhenDataIsPushed) // NOLINT
{
    TestableEventQueue eventQueueWithMaxSize2(2);
    auto expectedData = Common::ZeroMQWrapper::data_t({"one", "two", "three"});

    auto x = std::cv_status::timeout;
    (void)x;
    auto blockWhileWaitingForData = std::async(std::launch::async,
            [&eventQueueWithMaxSize2, &expectedData]
        {
            std::chrono::milliseconds before = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            std::optional<Common::ZeroMQWrapper::data_t> data = eventQueueWithMaxSize2.pop(1000);
            std::chrono::milliseconds after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            EXPECT_TRUE(data.has_value());
            EXPECT_EQ(data.value(), expectedData);
            return after.count() - before.count();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    eventQueueWithMaxSize2.push(expectedData);
    int duration = blockWhileWaitingForData.get();
    EXPECT_GE(duration, 100);
    EXPECT_NEAR(duration, 100, 10);
}

//TEST_F(TestEventQueue, testPushedDataIsCorrectlyQueuedAndReturnedWhenPopped) // NOLINT
//{
//    EventQueueLib::IEventQueue* eventQueue = new EventQueueLib::EventQueue(3);
//    std::shared_ptr<EventQueueLib::IEventQueue> eventQueuePtr(eventQueue);
//    SubscriberLib::IEventQueuePusher* pusher = new SubscriberLib::EventQueuePusher(eventQueuePtr);
//    WriterLib::IEventQueuePopper* popper = new WriterLib::EventQueuePopper(eventQueuePtr);
//
//    auto data1 = Common::ZeroMQWrapper::data_t({"one", "two", "three"});
//    auto data2 = Common::ZeroMQWrapper::data_t({"four", "five", "six"});
//    auto data3 = Common::ZeroMQWrapper::data_t({"seven", "eight", "nine"});
//
//    pusher->push(data1);
//    pusher->push(data2);
//    pusher->push(data3);
//
//    ASSERT_EQ(data1, popper->getEvent(10));
//    ASSERT_EQ(data2, popper->getEvent(10));
//    ASSERT_EQ(data3, popper->getEvent(10));
//}