//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FANotifyHandlerMemoryAppenderUsingTests.h"

#include "common/ThreadRunner.h"
#include "datatypes/MockSysCalls.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"

#include <gtest/gtest.h>
#include <sstream>

using namespace ::testing;

class TestEventReaderThread : public FANotifyHandlerMemoryAppenderUsingTests
{
protected:
    void SetUp() override
    {
        m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
    }

    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
};

TEST_F(TestEventReaderThread, TestReaderExitsUsingPipe)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    struct pollfd fds[2]{};
    fds[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    int fanotifyFD = 123;
    sophos_on_access_process::fanotifyhandler::EventReaderThread eventReader(fanotifyFD, m_mockSysCallWrapper);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader");

    EXPECT_TRUE(waitForLog("Stopping the reading of FANotify events"));
}

TEST_F(TestEventReaderThread, TestReaderLogsIfFanotifySendsNoEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(Return(0));

    sophos_on_access_process::fanotifyhandler::EventReaderThread eventReader(fanotifyFD, m_mockSysCallWrapper);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader");

    EXPECT_TRUE(waitForLog("no event or error: 0"));
    EXPECT_TRUE(waitForLog("Stopping the reading of FANotify events"));
}

ACTION_P(AssignFanotifyEvent, param)
{
    *static_cast<struct fanotify_event_metadata*>(arg1) = param;
}

TEST_F(TestEventReaderThread, TestReaderReadsFanotifyEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata;
    metadata.pid = 2345;
    metadata.fd = 345;
    metadata.mask = FAN_CLOSE;
    metadata.vers = FANOTIFY_METADATA_VERSION;
    metadata.event_len = FAN_EVENT_METADATA_LEN;

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(AssignFanotifyEvent(metadata), Return(sizeof(metadata))));

    sophos_on_access_process::fanotifyhandler::EventReaderThread eventReader(fanotifyFD, m_mockSysCallWrapper);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader");

    EXPECT_TRUE(waitForLog("got event: size "));
    std::stringstream logMsg;
    logMsg << "On-close event from PID " << metadata.pid << " for FD " << metadata.fd;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of FANotify events"));
}
