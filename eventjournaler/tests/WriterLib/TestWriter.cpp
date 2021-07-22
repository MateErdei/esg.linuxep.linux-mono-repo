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

class TestWriter : public LogOffInitializedTests{};

using namespace EventWriterLib;

TEST_F(TestWriter, testWriterFinishesWritingQueueContentsAfterReceivingStop) // NOLINT
{
    JournalerCommon::Event fakeData = {JournalerCommon::EventType::THREAT_EVENT, "test data"};
    std::vector<uint8_t> encodedFakeData = EventJournal::encode(EventJournal::Detection {"TODO", fakeData.data});
    std::unique_ptr<EventWriterLib::IEventQueuePopper> fakePopperPtr(new FakePopper(fakeData, 10));

    MockJournalWriter*  mockJournalWriter = new StrictMock<MockJournalWriter>();
    std::unique_ptr<IEventJournalWriter> mockJournalWriterPtr(mockJournalWriter);

    EventWriterLib::EventWriterWorker writer(std::move(fakePopperPtr), std::move(mockJournalWriterPtr));

    EXPECT_CALL(*mockJournalWriter, insert(EventJournal::Subject::Detections, encodedFakeData)).Times(10);

    writer.start();
    writer.stop();

    ASSERT_FALSE(writer.getRunningStatus());
}
