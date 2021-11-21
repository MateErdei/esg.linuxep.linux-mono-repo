/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <gtest/gtest.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <thread>
#include "tests/Helpers/FakePopper.h"
#include "MockJournalWriter.h"
#include <modules/EventWriterWorkerLib/EventWriterWorker.h>
#include <modules/EventJournal/EventJournalWriter.h>
#include "MockEventQueuePopper.h"

class TestWriter : public LogOffInitializedTests {
};

using namespace EventWriterLib;

TEST_F(TestWriter, testWriterLogsWarningOnBadDataAndThenContinues) // NOLINT
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

    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);
    EXPECT_CALL(*mockPopper, getEvent(_)).WillRepeatedly(Invoke(getNextEvent));
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    std::atomic<int> getEventCallCount = 0;
    auto setGetEventHasBeenCalledAndReturnVoid = [&getEventCallCount](Subject, const std::vector<uint8_t>&){
        getEventCallCount++;
    };

    EXPECT_CALL(*mockPopper, getEvent(100)).WillRepeatedly(Invoke(getNextEvent));
    EXPECT_CALL(*mockJournalWriter, insert(_,_)).WillRepeatedly(Invoke(setGetEventHasBeenCalledAndReturnVoid));

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    while (getEventCallCount < 2)
    {
        usleep(1);
    }

    EXPECT_TRUE(writer.getRunningStatus());
    writer.stop();
    EXPECT_FALSE(writer.getRunningStatus());
    ASSERT_EQ(getEventCallCount,2);
}

TEST_F(TestWriter, testWriterFinishesWritingQueueContentsAfterReceivingStop) // NOLINT
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};

    std::vector<uint8_t> encodedFakeData = EventJournal::encode(
            EventJournal::Detection{
                    JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(JournalerCommon::EventType::THREAT_EVENT),
                    fakeData.data});
    std::unique_ptr<EventWriterLib::IEventQueuePopper> fakePopperPtr(new FakePopper(fakeData, 10));

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(fakePopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData)).Times(10);

    writer.start();
    while(!writer.getRunningStatus())
    {
        usleep(1);
    }
    writer.stop();

    ASSERT_FALSE(writer.getRunningStatus());
}

TEST_F(TestWriter, WriterStartStopStart) // NOLINT
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
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    std::atomic<bool> getEventHasBeenCalled = false;
    auto setGetEventHasBeenCalledAndReturnVoid = [&getEventHasBeenCalled](Subject, const std::vector<uint8_t>&){
      getEventHasBeenCalled = true;
    };

    auto getNullEvent = [](int timeout){
      usleep(timeout * 1000);
      return std::optional<JournalerCommon::Event>();
    };

    EXPECT_CALL(*mockPopper, getEvent(100))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent));

    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData))
        .WillRepeatedly(Invoke(setGetEventHasBeenCalledAndReturnVoid));

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!getEventHasBeenCalled)
    {
        usleep(1);
    }
    EXPECT_TRUE(writer.getRunningStatus());
    writer.stop();
    EXPECT_FALSE(writer.getRunningStatus());
    getEventHasBeenCalled = false;
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!getEventHasBeenCalled)
    {
        usleep(1);
    }
    EXPECT_TRUE(writer.getRunningStatus());
}

TEST_F(TestWriter, WriterStopFollowedByStopDoesNotThrow) // NOLINT
{
    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);
    EXPECT_FALSE(writer.getRunningStatus());
    ASSERT_NO_THROW(writer.stop());
    EXPECT_FALSE(writer.getRunningStatus());
    ASSERT_NO_THROW(writer.stop());
    EXPECT_FALSE(writer.getRunningStatus());
}

TEST_F(TestWriter, WriterStartFollowedByStartDoesNotThrow) // NOLINT
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
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    std::atomic<bool> getEventHasBeenCalled = false;
    auto setGetEventHasBeenCalledAndReturnVoid = [&getEventHasBeenCalled](Subject, const std::vector<uint8_t>&){
      getEventHasBeenCalled = true;
    };

    EXPECT_CALL(*mockPopper, getEvent(100)).WillOnce(Return(fakeData)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData))
        .WillRepeatedly(Invoke(setGetEventHasBeenCalledAndReturnVoid));

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!getEventHasBeenCalled)
    {
        usleep(1);
    }
    EXPECT_TRUE(writer.getRunningStatus());
    ASSERT_NO_THROW(writer.start());
    EXPECT_TRUE(writer.getRunningStatus());
}



TEST_F(TestWriter, WriterRestart) // NOLINT
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
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    std::atomic<bool> getEventHasBeenCalled = false;
    auto setGetEventHasBeenCalledAndReturnVoid = [&getEventHasBeenCalled](Subject, const std::vector<uint8_t>&){
      getEventHasBeenCalled = true;
    };

    auto getNullEvent = [](int timeout){
      usleep(timeout * 1000);
      return std::optional<JournalerCommon::Event>();
    };

    EXPECT_CALL(*mockPopper, getEvent(100))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent))
        .WillOnce(Return(fakeData))
        .WillOnce(Invoke(getNullEvent));

    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData))
        .WillRepeatedly(Invoke(setGetEventHasBeenCalledAndReturnVoid));

    EXPECT_FALSE(writer.getRunningStatus());
    writer.start();
    // Make sure we have really started, start launches a thread.
    while (!getEventHasBeenCalled)
    {
        usleep(1);
    }
    EXPECT_TRUE(writer.getRunningStatus());
    getEventHasBeenCalled = false;
    writer.restart();
    // Make sure we have really started, start launches a thread.
    while (!getEventHasBeenCalled)
    {
        usleep(1);
    }
    EXPECT_TRUE(writer.getRunningStatus());
}

TEST_F(TestWriter, NonTruncatedJournalFilesAreNotPruned) // NOLINT
{
    const std::string filename = "detections.bin";

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    EXPECT_CALL(*mockJournalWriter, readFileInfo(filename, _)).WillOnce(Return(true));

    writer.checkAndPruneTruncatedEvents(filename);
}

TEST_F(TestWriter, TruncatedJournalFilesArePruned) // NOLINT
{
    const std::string filename = "detections.bin";

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    EventJournal::FileInfo info;
    info.anyLengthErrors = true;

    EXPECT_CALL(*mockJournalWriter, readFileInfo(filename, _)).WillOnce(DoAll(SetArgReferee<1>(info),Return(true)));
    EXPECT_CALL(*mockJournalWriter, pruneTruncatedEvents(filename));

    writer.checkAndPruneTruncatedEvents(filename);
}

TEST_F(TestWriter, InvalidJournalFilesAreRemoved) // NOLINT
{
    const std::string filename = "detections.bin";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);
    MockEventQueuePopper* mockPopper = new NiceMock<MockEventQueuePopper>();
    std::unique_ptr<IEventQueuePopper> mockPopperPtr(mockPopper);
    auto pulse = std::make_shared<bool>(false);
    Heartbeat::HeartbeatPinger heartbeatPinger(pulse);
    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr), heartbeatPinger);

    EXPECT_CALL(*mockFileSystem, removeFile(filename));
    EXPECT_CALL(*mockJournalWriter, readFileInfo(filename, _)).WillOnce(Return(false));

    writer.checkAndPruneTruncatedEvents(filename);
}
