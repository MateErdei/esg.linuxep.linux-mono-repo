/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <gtest/gtest.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <thread>
#include "tests/Helpers/FakePopper.h"
#include "MockJournalWriter.h"
#include <modules/EventWriterWorkerLib/EventWriterWorker.h>
#include <modules/EventJournal/EventJournalWriter.h>
#include "MockEventQueuePopper.h"

class TestWriter : public LogOffInitializedTests {
};

using namespace EventWriterLib;

// TODO
// test list
//------------
// stop followed by stop
// start followed by a start
// start then stop
// restart
// start, pop events, save to journal - THIS IS ALREADY COVERED BY testWriterFinishesWritingQueueContentsAfterReceivingStop
// start, pop bad/malformed expect writer to stop (no throw)
// To add to plugin adapter tests, copy this one: PluginAdapterRestartsSubscriberIfItStops


TEST_F(TestWriter, testWriterLogsWarningOnBadDataAndThenContinues) // NOLINT
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};

    std::vector<uint8_t> encodedFakeData = EventJournal::encode(
        EventJournal::Detection{
            JournalerCommon::EventTypeToJournalJsonSubtypeMap.at(JournalerCommon::EventType::THREAT_EVENT),
            fakeData.data});
    std::unique_ptr<EventWriterLib::IEventQueuePopper> fakePopperPtr(new FakePopper(fakeData, 10));

    MockJournalWriter *mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);

    EventWriterLib::EventWriterWorker writer(std::move(fakePopperPtr), std::move(mockJournalWriterPtr));

    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData)).Times(10);

    writer.start();
    while(!writer.getRunningStatus())
    {
        usleep(1);
    }
    writer.stop();

    ASSERT_FALSE(writer.getRunningStatus());
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

    EventWriterLib::EventWriterWorker writer(std::move(fakePopperPtr), std::move(mockJournalWriterPtr));

    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData)).Times(10);

    writer.start();
    while(!writer.getRunningStatus())
    {
        usleep(1);
    }
    writer.stop();

    ASSERT_FALSE(writer.getRunningStatus());
}

TEST_F(TestWriter, WriterStartAndStop) // NOLINT
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

    EventWriterLib::EventWriterWorker writer(std::move(mockPopperPtr), std::move(mockJournalWriterPtr));

    std::atomic<bool> getEventHasBeenCalled = false;
    // todo name
    auto setGetEventHasBeenCalledAndReturnVoid = [&getEventHasBeenCalled](){
      getEventHasBeenCalled = true;
    };

    EXPECT_CALL(*mockPopper, getEvent(100)).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData))
    .WillOnce(Invoke(setGetEventHasBeenCalledAndReturnVoid))
    .WillRepeatedly(Return(std::nullopt));

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
}
