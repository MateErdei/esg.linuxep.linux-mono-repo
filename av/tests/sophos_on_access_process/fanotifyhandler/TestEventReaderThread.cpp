//Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "common/ThreadRunner.h"
#include "common/WaitForEvent.h"
#include "datatypes/MockSysCalls.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gtest/gtest.h>
#include <sstream>

using namespace ::testing;
using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;

namespace
{
    constexpr int FANOTIFY_FD = 123;

    class FakeFanotify : public IFanotifyHandler
    {
    public:
        void init() override {}
        void close() override {}

        [[nodiscard]] int getFd() const override
        {
            return FANOTIFY_FD;
        }
        [[nodiscard]] int markMount(const std::string&) const override
        {
            return 0;
        }
        [[nodiscard]] int cacheFd(const int&, const std::string&) const override
        {
            return 0;
        }
        [[nodiscard]] int clearCachedFiles() const override
        {
            return 0;
        }
    };
}

class TestEventReaderThread : public FanotifyHandlerMemoryAppenderUsingTests
{
protected:
    void SetUp() override
    {
        m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        m_scanRequestQueue = std::make_shared<ScanRequestQueue>();
        m_fakeFanotify = std::make_shared<FakeFanotify>();
        m_SmallScanRequestQueue = std::make_shared<ScanRequestQueue>(3);
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        const auto* pluginInstall = "/opt/sophos-spl/plugins/av";
        appConfig.setData("PLUGIN_INSTALL", pluginInstall);
            m_statbuf.st_uid = 321;
    }

    static fanotify_event_metadata getMetaData(
        uint64_t _mask = FAN_CLOSE_WRITE,
        uint8_t _ver = FANOTIFY_METADATA_VERSION,
        int32_t _fd = 345,
        int32_t _pid = 1999999999)
    {
        return {
            .event_len = FAN_EVENT_METADATA_LEN, .vers = _ver, .reserved = 0,
            .metadata_len = FAN_EVENT_METADATA_LEN, .mask = _mask, .fd = _fd, .pid = _pid
        };
    }

    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper;
    ScanRequestQueueSharedPtr m_scanRequestQueue;
    ScanRequestQueueSharedPtr m_SmallScanRequestQueue;
    fs::path m_pluginInstall = "/opt/sophos-spl/plugins/av";
    IFanotifyHandlerSharedPtr m_fakeFanotify;
        struct ::stat m_statbuf {};
};

TEST_F(TestEventReaderThread, TestReaderExitsUsingPipe)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderLogsErrorIfFanotifySendsNoEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _)).WillOnce(Return(0));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("no event or error: 0"));
    EXPECT_TRUE(waitForLog("Failed to handle Fanotify event, stopping the reading of more events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderReadsOnCloseFanotifyEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillOnce(readlinkReturnPath(filePath));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ") and UID " << m_statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderReadsOnOpenFanotifyEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData(FAN_OPEN);
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-open event for " << filePath << " from Process (PID=" << metadata.pid << ") and UID " << m_statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderReadsOnOpenFanotifyEventAfterRestart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData(FAN_OPEN);
    const char* filePath1 = "/tmp/test";
    const char* filePath2 = "/tmp/test/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).Times(2)
        .WillRepeatedly(readReturnsStruct(metadata));
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillOnce(readlinkReturnPath(filePath1))
        .WillOnce(readlinkReturnPath(filePath2));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).Times(2)
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg1;
    logMsg1 << "On-open event for " << filePath1 << " from Process (PID=" << metadata.pid << ") and UID " << m_statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg1.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);

    eventReaderThread.requestStopIfNotStopped();
    eventReaderThread.startIfNotStarted();

    std::stringstream logMsg2;
    logMsg2 << "On-open event for " << filePath2 << " from Process (PID=" << metadata.pid << ") and UID " << m_statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg2.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 2);
}

TEST_F(TestEventReaderThread, TestReaderLogsUnexpectedFanotifyEventType)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData(FAN_ONDIR);
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "unknown operation mask: " << std::hex << metadata.mask;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSetUnknownPathIfReadLinkFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for unknown from Process (PID=" << metadata.pid << ") and UID " << m_statbuf.st_uid;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderSetInvalidUidIfStatFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(Return(-1));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ") and UID unknown";
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderExitsIfFanotifyProtocolVersionIsTooOld)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = FANOTIFY_FD;

    auto metadata = getMetaData(FAN_CLOSE, 2);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
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

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData(FAN_CLOSE, FANOTIFY_METADATA_VERSION, -1);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Got fanotify metadata event without fd"));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsWithSoapdPid)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData(FAN_CLOSE, FANOTIFY_METADATA_VERSION, 345, getpid());

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsInPluginLogDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/opt/sophos-spl/plugins/av/log/soapd.log";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsExcludedPaths)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back("/excluded/");
    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));
    const char* filePath = "/excluded/eicar.com";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    eventReader->setExclusions(exclusions);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderDoesntInvalidateFd)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _)).WillOnce(readReturnsStruct(metadata));
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));


    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    eventReaderThread.requestStopIfNotStopped();

    EXPECT_EQ(m_scanRequestQueue->size(), 1);
    auto popItem = m_scanRequestQueue->pop();
    EXPECT_TRUE(popItem != nullptr);
    EXPECT_NE(popItem->getFd(), -1);
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderLogsFanotifyQueueOverflow)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData(FAN_Q_OVERFLOW, FANOTIFY_METADATA_VERSION, FAN_NOFD);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _))
        .WillOnce(readReturnsStruct(metadata));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_scanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Fanotify queue overflowed, some files will not be scanned."));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderLogsQueueIsFull)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_SmallScanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    EXPECT_EQ(m_SmallScanRequestQueue->size(), 3);
}

TEST_F(TestEventReaderThread, TestReaderDoesntLogQueueIsFullWhenIsAlreadyFull)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_SmallScanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_TRUE(appenderContains("Failed to add scan request to queue, on-access scanning queue is full."));
    EXPECT_FALSE(appenderContainsCount("Failed to add scan request to queue, on-access scanning queue is full.", 2));
    EXPECT_EQ(m_SmallScanRequestQueue->size(), 3);
}

TEST_F(TestEventReaderThread, TestReaderLogsQueueIsFullWhenItFillsSecondTime)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent eventReaderGuard;

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&eventReaderGuard, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(1, POLLIN)))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_SmallScanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    m_SmallScanRequestQueue->restart();
    eventReaderGuard.onEventNoArgs();
    EXPECT_TRUE(waitForLogMultiple("Failed to add scan request to queue, on-access scanning queue is full.", 2, 100ms));
    EXPECT_TRUE(appenderContains("Queue is no longer full. Number of events dropped: 1"));
}

TEST_F(TestEventReaderThread, TestReaderLogsCorrectlyWhenQueueIsNoLongerFullButNotEmpty)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent eventReaderGuard;

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&eventReaderGuard, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(1, POLLIN)))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_SmallScanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    m_SmallScanRequestQueue->pop();
    EXPECT_EQ(m_SmallScanRequestQueue->size(), 2);
    eventReaderGuard.onEventNoArgs();
    EXPECT_TRUE(waitForLogMultiple("Failed to add scan request to queue, on-access scanning queue is full.", 2, 100ms));
    EXPECT_TRUE(appenderContains("Queue is no longer full. Number of events dropped: 2"));
}

TEST_F(TestEventReaderThread, TestReaderLogsManyEventsMissed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent eventReaderGuard;

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&eventReaderGuard, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(1, POLLIN)))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_SmallScanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    m_SmallScanRequestQueue->restart();

    eventReaderGuard.onEventNoArgs();
    EXPECT_TRUE(waitForLog("Queue is no longer full. Number of events dropped: 2", 100ms));
}

TEST_F(TestEventReaderThread, TestReaderResetsMissedEventCountAfterStopStart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent eventReaderGuard;

    int fanotifyFD = FANOTIFY_FD;
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&eventReaderGuard, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(0, POLLIN)))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&eventReaderGuard, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(0, POLLIN)));

    EXPECT_CALL(*m_mockSysCallWrapper, read(fanotifyFD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    auto eventReader = std::make_shared<EventReaderThread>(m_fakeFanotify, m_mockSysCallWrapper, m_pluginInstall, m_SmallScanRequestQueue);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    //Wait for queue to fill before restarting
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));

    //Restart
    eventReaderGuard.onEventNoArgs();
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    eventReaderThread.requestStopIfNotStopped();
    eventReaderGuard.clear();
    m_SmallScanRequestQueue->restart();
    eventReaderThread.startIfNotStarted();

    //Wait for queue to fill again and make critical observation
    EXPECT_TRUE(waitForLogMultiple("Failed to add scan request to queue, on-access scanning queue is full.", 2));
    EXPECT_FALSE(appenderContains("Queue is no longer full. Number of events dropped: 1"));

    //End test
    eventReaderGuard.onEventNoArgs();
}