/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <gtest/gtest.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <modules/WriterLib/EventQueuePopper.h>
#include <thread>
#include <future>
#include "tests/Helpers/FakePopper.h"
#include "MockJournalWriter.h"
//#include writer

class TestWriter : public LogOffInitializedTests{};

using namespace WriterLib;

TEST_F(TestWriter, testWriterFinishesWritingQueueContentsAfterReceivingStop) // NOLINT
{
    Common::ZeroMQWrapper::data_t dataOne = {"first", "test", "data"};
    int timeout = 1000;
    FakePopper fakePopper(dataOne, 10);
    // Pointer?
    WriterLib::WriterObject writer(fakePopper/*, timeout*/);
    MockJournalWriter*  mockJournalWriter = new StrictMock<MockJournalWriter>();

    EXPECT_CALL(*mockJournalWriter, write(dataOne)).Times(10);

    writer.start();
    auto unblockTask = std::async(std::launch::async, [&fakePopper] { fakePopper.setBlock(false); });
    writer.stop();
    unblockTask.get();

    ASSERT_FALSE(writer.getRunningStatus());
    ASSERT_TRUE(fakePopper.queueEmpty());
}
