//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "common/ThreadRunner.h"
#include "datatypes/MockSysCalls.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gtest/gtest.h>
#include <sstream>

using namespace ::testing;
using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;

class TestEventReaderThread : public FanotifyHandlerMemoryAppenderUsingTests
{
protected:
    void SetUp() override
    {
        m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        m_scanRequestQueue = std::make_shared<ScanRequestQueue>();
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        const auto* pluginInstall = "/opt/sophos-spl/plugins/av";
        appConfig.setData("PLUGIN_INSTALL", pluginInstall);
    }

    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
    ScanRequestQueueSharedPtr m_scanRequestQueue;
    fs::path m_pluginInstall = "/opt/sophos-spl/plugins/av";
};

TEST_F(TestEventReaderThread, TestReaderExitsUsingPipe)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    struct pollfd fds[2]{};
    fds[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));

    int fanotifyFD = 123;
    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderLogsErrorIfFanotifySendsNoEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;

    struct pollfd fds[2]{};
    fds[1].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(Return(0));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("no event or error: 0"));
    EXPECT_TRUE(waitForLog("Failed to handle Fanotify event, stopping the reading of more events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderReadsOnCloseFanotifyEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_CLOSE, .fd = 345, .pid = 2345 };
    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(DoAll(SetArrayArgument<1>(filePath, filePath + strlen(filePath) + 1), Return(strlen(filePath) + 1)));
    struct ::stat statbuf{};
    statbuf.st_uid = 1;
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(DoAll(SetArgPointee<1>(statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ") and UID " << statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderReadsOnOpenFanotifyEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_OPEN, .fd = 345, .pid = 2345 };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(DoAll(SetArrayArgument<1>(filePath, filePath + strlen(filePath) + 1), Return(strlen(filePath) + 1)));

    struct ::stat statbuf{};
    statbuf.st_uid = 1;
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(DoAll(SetArgPointee<1>(statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-open event for " << filePath << " from Process (PID=" << metadata.pid << ") and UID " << statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderReadsOnOpenFanotifyEventAfterRestart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_OPEN, .fd = 345, .pid = 2345 };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).Times(2).WillRepeatedly(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));
    const char* filePath1 = "/tmp/test";
    const char* filePath2 = "/tmp/test/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillOnce(DoAll(SetArrayArgument<1>(filePath1, filePath1 + strlen(filePath1) + 1), Return(strlen(filePath1) + 1)))
        .WillOnce(DoAll(SetArrayArgument<1>(filePath2, filePath2 + strlen(filePath2) + 1), Return(strlen(filePath2) + 1)));
    struct ::stat statbuf{};
    statbuf.st_uid = 1;
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).Times(2).WillRepeatedly(DoAll(SetArgPointee<1>(statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg1;
    logMsg1 << "On-open event for " << filePath1 << " from Process (PID=" << metadata.pid << ") and UID " << statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg1.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);

    eventReaderThread.requestStopIfNotStopped();
    eventReaderThread.startIfNotStarted();

    std::stringstream logMsg2;
    logMsg2 << "On-open event for " << filePath2 << " from Process (PID=" << metadata.pid << ") and UID " << statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg2.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 2);
}

TEST_F(TestEventReaderThread, TestReaderLogsUnexpectedFanotifyEventType)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_ONDIR, .fd = 345, .pid = 2345 };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(DoAll(SetArrayArgument<1>(filePath, filePath + strlen(filePath) + 1), Return(strlen(filePath) + 1)));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "unknown operation mask: " << std::hex << metadata.mask;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
}

TEST_F(TestEventReaderThread, TestReaderSetInvalidUidIfStatFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_CLOSE, .fd = 345, .pid = 2345 };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));
    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(DoAll(SetArrayArgument<1>(filePath, filePath + strlen(filePath) + 1), Return(strlen(filePath) + 1)));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(Return(-1));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    uid_t invalidUid = static_cast<uid_t>(-1);
    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ") and UID " << invalidUid;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderExitsIfFanotifyProtocolVersionIsTooOld)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = 2, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_CLOSE, .fd = 345, .pid = 2345 };

    struct pollfd fds[2]{};
    fds[1].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds, fds+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "fanotify wrong protocol version " << metadata.vers;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Failed to handle Fanotify event, stopping the reading of more events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsWithoutFD)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_CLOSE, .fd = -1, .pid = 2345 };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Got fanotify metadata event without fd"));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsWithSoapdPid)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_CLOSE, .fd = 345, .pid = getpid() };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));
    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(DoAll(SetArrayArgument<1>(filePath, filePath + strlen(filePath) + 1), Return(strlen(filePath) + 1)));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Event pid matches On Access pid, skipping event"));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsInPluginLogDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_CLOSE, .fd = 345, .pid = 2345 };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));
    const char* filePath = "/opt/sophos-spl/plugins/av/log/soapd.log";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(DoAll(SetArrayArgument<1>(filePath, filePath + strlen(filePath) + 1), Return(strlen(filePath) + 1)));

    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderDoesntInvalidateFd)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = 123;
    struct fanotify_event_metadata metadata = {
        .event_len = FAN_EVENT_METADATA_LEN, .vers = FANOTIFY_METADATA_VERSION, .reserved = 0, .metadata_len = FAN_EVENT_METADATA_LEN,
        .mask = FAN_CLOSE_WRITE, .fd = 345, .pid = 2345 };

    struct pollfd fds1[2]{};
    fds1[1].revents = POLLIN;
    struct pollfd fds2[2]{};
    fds2[0].revents = POLLIN;
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(DoAll(SetArrayArgument<0>(fds1, fds1+2), Return(1)))
        .WillOnce(DoAll(SetArrayArgument<0>(fds2, fds2+2), Return(1)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(DoAll(
        Invoke([metadata] (int, void* arg2, size_t) { *static_cast<struct fanotify_event_metadata*>(arg2) = metadata; }),
        Return(sizeof(metadata))));
    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(DoAll(SetArrayArgument<1>(filePath, filePath + strlen(filePath) + 1), Return(strlen(filePath) + 1)));
    struct ::stat statbuf{};
    statbuf.st_uid = 1;
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(DoAll(SetArgPointee<1>(statbuf), Return(0)));


    auto eventReader = std::make_shared<EventReaderThread>(fanotifyFD, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    eventReaderThread.requestStopIfNotStopped();

    EXPECT_EQ(m_scanRequestQueue->size(), 1);
    EXPECT_NE(m_scanRequestQueue->pop()->getFd(), -1);
}