// Copyright 2021-2023 Sophos Limited. All rights reserved.


#include "MockEventQueuePopper.h"
#include "MockJournalWriter.h"

#ifdef SPL_BAZEL
#include "eventjournaler/tests/Helpers/FakePopper.h"
#include "EventWriterWorkerLib/EventWriterWorker.h"
#include "EventJournal/EventJournalWriter.h"
#include "Heartbeat/MockHeartbeatPinger.h"
#include "JournalerCommon/TimeConsts.h"

#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#else
#include "tests/Helpers/FakePopper.h"
#include "modules/EventWriterWorkerLib/EventWriterWorker.h"
#include "modules/EventJournal/EventJournalWriter.h"
#include "modules/Heartbeat/MockHeartbeatPinger.h"
#include "modules/JournalerCommon/TimeConsts.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#endif

#include "Common/FileSystem/IFileSystem.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <gtest/gtest.h>

#include <thread>

using namespace EventQueueLib;
using namespace EventWriterLib;

namespace
{
    class TestWriter : public LogOffInitializedTests
    {
    };

    constexpr const auto DEFAULT_QUEUE_SLEEP_INTERVAL_MS = JournalerCommon::DEFAULT_QUEUE_SLEEP_INTERVAL_MS;
}

TEST_F(TestWriter, testWriterLogsWarningOnBadDataAndThenContinues)
{
    JournalerCommon::Event goodData = {JournalerCommon::EventType::THREAT_EVENT, "good data"};
    JournalerCommon::Event badData = {JournalerCommon::EventType::UNKNOWN, "bad data"};

    std::vector<uint8_t> encodedGoodData = EventJournal::encode(
        EventJournal::Detection{
            JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(goodData.type),
            goodData.data});

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);

    std::vector<JournalerCommon::Event> mockSocketValues = {goodData, badData, goodData};
    auto getNextEvent = [&mockSocketValues](int timeout){
        usleep(timeout);
        if (mockSocketValues.empty())
        {
            return std::optional<JournalerCommon::Event>();
        }
        JournalerCommon::Event fakeEventData = mockSocketValues.back();
        mockSocketValues.pop_back();
        return std::optional<JournalerCommon::Event>(fakeEventData);
    };

    auto mockPopper = std::make_shared<NiceMock<MockEventQueuePopper>>();
    EXPECT_CALL(*mockPopper, pop(_)).WillRepeatedly(Invoke(getNextEvent));
    EXPECT_CALL(*mockPopper, pop(DEFAULT_QUEUE_SLEEP_INTERVAL_MS)).WillRepeatedly(Invoke(getNextEvent));

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();

    std::atomic<int> popCallCount = 0;
    auto setpopHasBeenCalledAndReturnVoid = [&popCallCount](Subject, const std::vector<uint8_t>&){
        popCallCount++;
    };

    EXPECT_CALL(*mockJournalWriter, insert(_,_)).WillRepeatedly(Invoke(setpopHasBeenCalledAndReturnVoid));

    EventWriterLib::EventWriterWorker writer(std::move(mockPopper), std::move(mockJournalWriterPtr), heartbeatPinger);

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    while (popCallCount < 2)
    {
        usleep(1);
    }

    EXPECT_TRUE(writer.getRunningStatus());
    writer.stop();
    EXPECT_FALSE(writer.getRunningStatus());
    ASSERT_EQ(popCallCount,2);
}

TEST_F(TestWriter, testWriterFinishesWritingQueueContentsAfterReceivingStop)
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};

    std::vector<uint8_t> encodedFakeData = EventJournal::encode(
            EventJournal::Detection{
                    JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(JournalerCommon::EventType::THREAT_EVENT),
                    fakeData.data});
    auto fakePopperPtr = std::make_unique<FakePopper>(fakeData, 10, 50);

    auto mockJournalWriter = std::make_unique<StrictMock<MockJournalWriter>>();
    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData)).Times(10);

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();

    EventWriterLib::EventWriterWorker writer(std::move(fakePopperPtr), std::move(mockJournalWriter), std::move(heartbeatPinger));

    writer.start();
    while(!writer.getRunningStatus())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    writer.stop();

    ASSERT_FALSE(writer.getRunningStatus());
}

TEST_F(TestWriter, StartStopStart)
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};

    std::vector<uint8_t> encodedFakeData = EventJournal::encode(
        EventJournal::Detection{
            JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(JournalerCommon::EventType::THREAT_EVENT),
            fakeData.data});

    auto mockJournalWriterPtr = std::make_unique<StrictMock<MockJournalWriter>>();
    std::atomic<bool> popHasBeenCalled = false;
    auto setpopHasBeenCalledAndReturnVoid = [&popHasBeenCalled](Subject, const std::vector<uint8_t>&){
        popHasBeenCalled = true;
    };
    EXPECT_CALL(*mockJournalWriterPtr, insert(EventJournal::Subject::Detections, encodedFakeData))
        .WillRepeatedly(Invoke(setpopHasBeenCalledAndReturnVoid));

    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    auto getNullEvent = [](int){
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        return std::optional<JournalerCommon::Event>();
    };

    EXPECT_CALL(*mockPopper, pop(DEFAULT_QUEUE_SLEEP_INTERVAL_MS))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent));

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!popHasBeenCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    EXPECT_TRUE(writer.getRunningStatus());
    writer.stop();
    EXPECT_FALSE(writer.getRunningStatus());
    popHasBeenCalled = false;
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!popHasBeenCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    EXPECT_TRUE(writer.getRunningStatus());
}

TEST_F(TestWriter, WriterStopFollowedByStopDoesNotThrow)
{
    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);
    EXPECT_FALSE(writer.getRunningStatus());
    ASSERT_NO_THROW(writer.stop());
    EXPECT_FALSE(writer.getRunningStatus());
    ASSERT_NO_THROW(writer.stop());
    EXPECT_FALSE(writer.getRunningStatus());
}

TEST_F(TestWriter, WriterStartFollowedByStartDoesNotThrow)
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};

    std::vector<uint8_t> encodedFakeData = EventJournal::encode(
        EventJournal::Detection{
            JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(JournalerCommon::EventType::THREAT_EVENT),
            fakeData.data});

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);
    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    std::atomic<bool> popHasBeenCalled = false;
    auto setpopHasBeenCalledAndReturnVoid = [&popHasBeenCalled](Subject, const std::vector<uint8_t>&){
      popHasBeenCalled = true;
    };

    EXPECT_CALL(*mockPopper, pop(DEFAULT_QUEUE_SLEEP_INTERVAL_MS)).WillOnce(Return(fakeData)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData))
        .WillRepeatedly(Invoke(setpopHasBeenCalledAndReturnVoid));

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!popHasBeenCalled)
    {
        usleep(1);
    }
    EXPECT_TRUE(writer.getRunningStatus());
    ASSERT_NO_THROW(writer.start());
    EXPECT_TRUE(writer.getRunningStatus());
}



TEST_F(TestWriter, WriterRestart)
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};

    std::vector<uint8_t> encodedFakeData = EventJournal::encode(
        EventJournal::Detection{
            JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(JournalerCommon::EventType::THREAT_EVENT),
            fakeData.data});

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    std::atomic<bool> popHasBeenCalled = false;
    auto setpopHasBeenCalledAndReturnVoid = [&popHasBeenCalled](Subject, const std::vector<uint8_t>&){
      popHasBeenCalled = true;
    };

    auto getNullEvent = [](int timeout){
        std::ignore = timeout;
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        return std::optional<JournalerCommon::Event>();
    };

    EXPECT_CALL(*mockPopper, pop(DEFAULT_QUEUE_SLEEP_INTERVAL_MS))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent));

    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData))
        .WillRepeatedly(Invoke(setpopHasBeenCalledAndReturnVoid));

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!popHasBeenCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    EXPECT_TRUE(writer.getRunningStatus());
    popHasBeenCalled = false;
    writer.restart();
    // Make sure we have really started, start launches a thread.
    while (!popHasBeenCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    EXPECT_TRUE(writer.getRunningStatus());
}

TEST_F(TestWriter, NonTruncatedJournalFilesAreNotPruned)
{
    const std::string filename = "detections.bin";

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    EXPECT_CALL(*mockJournalWriter, readFileInfo(filename, _)).WillOnce(Return(true));

    writer.checkAndPruneTruncatedEvents(filename);
}

TEST_F(TestWriter, TruncatedJournalFilesArePruned)
{
    const std::string filename = "detections.bin";

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    EventJournal::FileInfo info;
    info.anyLengthErrors = true;

    EXPECT_CALL(*mockJournalWriter, readFileInfo(filename, _)).WillOnce(DoAll(SetArgReferee<1>(info),Return(true)));
    EXPECT_CALL(*mockJournalWriter, pruneTruncatedEvents(filename));

    writer.checkAndPruneTruncatedEvents(filename);
}

TEST_F(TestWriter, InvalidJournalFilesAreRemoved)
{
    const std::string filename = "detections.bin";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto heartbeatPinger = std::make_shared<Heartbeat::HeartbeatPinger>();
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    EXPECT_CALL(*mockFileSystem, removeFile(filename));
    EXPECT_CALL(*mockJournalWriter, readFileInfo(filename, _)).WillOnce(Return(false));

    writer.checkAndPruneTruncatedEvents(filename);
}

TEST_F(TestWriter, writerSetsDroppedEventsMaxOnHeartbeatPinger)
{
    const std::string filename = "detections.bin";

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto mockHeartbeatPinger = std::make_shared<Heartbeat::MockHeartbeatPinger>();
    EXPECT_CALL(*mockHeartbeatPinger, setDroppedEventsMax(6));
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), mockHeartbeatPinger);
}

TEST_F(TestWriter, testWriterPingsHeartbeatRepeatedlyInWriterThread)
{
    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);

    auto mockHeartbeatPinger = std::make_shared<StrictMock<Heartbeat::MockHeartbeatPinger>>();
    EXPECT_CALL(*mockHeartbeatPinger, setDroppedEventsMax(6));
    EXPECT_CALL(*mockHeartbeatPinger, ping).Times(AtLeast(2));
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), mockHeartbeatPinger);

    writer.start();
    usleep(100000);
}

TEST_F(TestWriter, testWriterPushesDroppedEventOnFailedWrite)
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};

    std::vector<uint8_t> encodedFakeData = EventJournal::encode(
            EventJournal::Detection{
                    JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(JournalerCommon::EventType::THREAT_EVENT),
                    fakeData.data});

    auto fakePopperPtr = std::make_unique<FakePopper>(fakeData, 2, 25);

    auto mockJournalWriterPtr = std::make_unique<StrictMock<MockJournalWriter>>();
    EXPECT_CALL(*mockJournalWriterPtr, insert(EventJournal::Subject::Detections, encodedFakeData))
        .WillRepeatedly(Throw(std::exception()));

    auto mockHeartbeatPinger = std::make_shared<StrictMock<Heartbeat::MockHeartbeatPinger>>();
    EXPECT_CALL(*mockHeartbeatPinger, setDroppedEventsMax(6));
    EXPECT_CALL(*mockHeartbeatPinger, ping).Times(AnyNumber());
    EXPECT_CALL(*mockHeartbeatPinger, pushDroppedEvent).Times(2);

    EventWriterLib::EventWriterWorker writer(
        std::move(fakePopperPtr),
        std::move(mockJournalWriterPtr),
        mockHeartbeatPinger,
        1000);

    writer.start();
    while(!writer.getRunningStatus())
    {
        usleep(1);
    }
    writer.stop();
}