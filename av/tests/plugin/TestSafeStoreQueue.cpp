// Copyright 2022, Sophos Limited.  All rights reserved.

#include <Common/FileSystem/IFileSystem.h>
#include <gtest/gtest.h>
#include <Common/Helpers/LogInitializedTests.h>

#include "pluginimpl/QueueSafeStoreTask.h"
#include "scan_messages/ThreatDetected.h"

#include <future>
#include <thread>


class TestSafeStoreQueue : public LogOffInitializedTests{};

//TEST_F(TestSafeStoreQueue, TestSafeStoreQueueIsFullReturnsTrueWhenQueueIsFull) // NOLINT
//{
//    TestableEventQueue eventQueueWithMaxSize1(1);
//    auto queueWithNoItems =  createQueueOfSize(0);
//    auto queueWithOneItem =  createQueueOfSize(1);
//    auto queueWithTwoItem =  createQueueOfSize(2);
//    eventQueueWithMaxSize1.setQueue(queueWithNoItems);
//    EXPECT_FALSE(eventQueueWithMaxSize1.isQueueFull());
//    eventQueueWithMaxSize1.setQueue(queueWithOneItem);
//    EXPECT_TRUE(eventQueueWithMaxSize1.isQueueFull());
//    eventQueueWithMaxSize1.setQueue(queueWithTwoItem);
//    EXPECT_TRUE(eventQueueWithMaxSize1.isQueueFull());
//
//    TestableEventQueue eventQueueWithMaxSize5(5);
//    auto queueWith5Items =  createQueueOfSize(5);
//    auto queueWith3Items =  createQueueOfSize(3);
//    eventQueueWithMaxSize5.setQueue(queueWithNoItems);
//    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueFull());
//    eventQueueWithMaxSize5.setQueue(queueWith5Items);
//    EXPECT_TRUE(eventQueueWithMaxSize5.isQueueFull());
//    eventQueueWithMaxSize5.setQueue(queueWith3Items);
//    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueFull());
//}
//
//TEST_F(TestSafeStoreQueue, TestSafeStoreQueueIsEmptyReturnsTrueWhenQueueIsEmpty) // NOLINT
//{
//    TestableEventQueue eventQueueWithMaxSize5(5);
//    auto queueWith0Items =  createQueueOfSize(0);
//    auto queueWith1Items =  createQueueOfSize(1);
//    auto queueWith5Items =  createQueueOfSize(5);
//    auto queueWith3Items =  createQueueOfSize(3);
//
//    eventQueueWithMaxSize5.setQueue(queueWith0Items);
//    EXPECT_TRUE(eventQueueWithMaxSize5.isQueueEmpty());
//    eventQueueWithMaxSize5.setQueue(queueWith1Items);
//    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueEmpty());
//    eventQueueWithMaxSize5.setQueue(queueWith5Items);
//    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueEmpty());
//    eventQueueWithMaxSize5.setQueue(queueWith3Items);
//    EXPECT_FALSE(eventQueueWithMaxSize5.isQueueEmpty());
//}
//
//
//TEST_F(TestSafeStoreQueue, TestSafeStoreQueuePopBlocksUntilToldToStop) // NOLINT
//{
//    TestableEventQueue eventQueueWithMaxSize2(2);
//    int timeout = 100;
//
//    std::chrono::milliseconds before = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//    std::optional<JournalerCommon::Event> emptyOptionalData = eventQueueWithMaxSize2.pop(timeout);
//    std::chrono::milliseconds after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//
//    EXPECT_FALSE(emptyOptionalData.has_value());
//    int duration = after.count() - before.count();
//    EXPECT_GE(duration, timeout);
//    EXPECT_NEAR(duration, timeout, 10);
//}
//
//TEST_F(TestSafeStoreQueue, TestSafeStoreQueuePopReturnsValueImmediatelyWhenThereIsDataToPop) // NOLINT
//{
//    TestableEventQueue eventQueueWithMaxSize2(2);
//    auto mockedQueue = std::queue<JournalerCommon::Event>();
//    JournalerCommon::Event expectedData1 {JournalerCommon::EventType::THREAT_EVENT, "fake data one"};
//    JournalerCommon::Event expectedData2 {JournalerCommon::EventType::THREAT_EVENT, "fake data two"};
//    mockedQueue.push(expectedData1);
//    mockedQueue.push(expectedData2);
//    eventQueueWithMaxSize2.setQueue(mockedQueue);
//
//
//    std::chrono::milliseconds before1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//    std::optional<JournalerCommon::Event> data1 = eventQueueWithMaxSize2.pop(1000);
//    std::chrono::milliseconds after1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//
//    std::chrono::milliseconds before2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//    std::optional<JournalerCommon::Event> data2 = eventQueueWithMaxSize2.pop(1000);
//    std::chrono::milliseconds after2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//
//    EXPECT_EQ(data1->data, expectedData1.data);
//    EXPECT_EQ(data2->data, expectedData2.data);
//    EXPECT_NEAR(after1.count() - before1.count(), 0, 10);
//    EXPECT_NEAR(after2.count() - before2.count(), 0, 10);
//}
//
//TEST_F(TestSafeStoreQueue, TestSafeStoreQueuePopBlocksDuringTimeoutBeforeUnblockingAndReturningValueWhenDataIsPushed) // NOLINT
//{
//    TestableEventQueue eventQueueWithMaxSize2(2);
//    JournalerCommon::Event expectedData {JournalerCommon::EventType::THREAT_EVENT, "fake data"};
//
//    auto blockWhileWaitingForData = std::async(std::launch::async,
//                                               [&eventQueueWithMaxSize2, &expectedData]
//                                               {
//                                                   std::chrono::milliseconds before = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//                                                   std::optional<JournalerCommon::Event> data = eventQueueWithMaxSize2.pop(1000);
//                                                   std::chrono::milliseconds after = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//                                                   EXPECT_TRUE(data.has_value());
//                                                   EXPECT_EQ(data->data, expectedData.data);
//                                                   return after.count() - before.count();
//                                               });
//
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));
//    eventQueueWithMaxSize2.push(expectedData);
//    int duration = blockWhileWaitingForData.get();
//    EXPECT_GE(duration, 100);
//    EXPECT_NEAR(duration, 100, 10);
//}
//

TEST_F(TestSafeStoreQueue, testPushedDataIsCorrectlyQueuedAndReturnedWhenPopped) // NOLINT
{
    Plugin::QueueSafeStoreTask queue;

    scan_messages::ThreatDetected data1(
        "root",
        1,
        scan_messages::E_VIRUS_THREAT_TYPE,
        "threatName",
        scan_messages::E_SCAN_TYPE_UNKNOWN,
        scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE,
        "/path",
        scan_messages::E_SMT_THREAT_ACTION_UNKNOWN,
        "sha256",
        "threatId",
        datatypes::AutoFd());


    scan_messages::ThreatDetected data1clone(
        "root",
        1,
        scan_messages::E_VIRUS_THREAT_TYPE,
        "threatName",
        scan_messages::E_SCAN_TYPE_UNKNOWN,
        scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE,
        "/path",
        scan_messages::E_SMT_THREAT_ACTION_UNKNOWN,
        "sha256",
        "threatId",
        datatypes::AutoFd());


    queue.push(std::move(data1));
//    pusher->handleEvent(data2);
//    pusher->handleEvent(data3);
    auto poppedData1 = queue.pop();

    ASSERT_EQ(poppedData1, data1clone);
//    ASSERT_EQ(data2.data, popper->getEvent(10)->data);
//    ASSERT_EQ(data3.data, popper->getEvent(10)->data);
}