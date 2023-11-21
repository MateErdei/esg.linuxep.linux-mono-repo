// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#define TEST_PUBLIC public

#include "common/ThreadRunner.h"
#include "sophos_on_access_process/local_settings/OnAccessProductConfigDefaults.h"
#include "sophos_on_access_process/fanotifyhandler/EventReaderThread.h"
#include "sophos_on_access_process/OnAccessTelemetryUtility/OnAccessTelemetryUtility.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include "tests/common/WaitForEvent.h"
#include "tests/mount_monitor/mountinfoimpl/MockDeviceUtil.h"
#include "Common/Helpers/MockSysCalls.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string_view>

using namespace ::testing;
using namespace sophos_on_access_process::fanotifyhandler;
using namespace sophos_on_access_process::onaccessimpl;
using namespace sophos_on_access_process::local_settings;
using namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry;


#define LOG(x) LOG4CPLUS_INFO(getLogger(), x)

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
        [[nodiscard]] int markMount(const std::string&, bool, bool) override
        {
            return 0;
        }
        [[nodiscard]] int unmarkMount(const std::string&) override
        {
            return 0;
        }
        [[nodiscard]] int cacheFd(const int&, const std::string&, bool) const override
        {
            return 0;
        }
        [[nodiscard]] int uncacheFd(const int&, const std::string&) const override
        {
            return 0;
        }
        [[nodiscard]] int clearCachedFiles() const override
        {
            return 0;
        }
        void updateComplete() override {};
    };

    class MockFanotify : public FakeFanotify
    {
    public:
        MOCK_METHOD(int, cacheFd, (const int&, const std::string&, bool), (const, override));
    };
}

ACTION(fstatInodeIncrementing)
{
    static ino_t inode = 50;
    arg1->st_ino = inode++;
    return 0;
}

class TestEventReaderThread : public FanotifyHandlerMemoryAppenderUsingTests
{
protected:
    void SetUp() override
    {
        m_mockSysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();
        m_scanRequestQueue = std::make_shared<ScanRequestQueue>(defaultMaxScanQueueSize);
        m_fakeFanotify = std::make_shared<FakeFanotify>();
        m_smallScanRequestQueue = std::make_shared<ScanRequestQueue>(3);
        m_telemetryUtility = std::make_shared<OnAccessTelemetryUtility>();
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("PLUGIN_INSTALL", m_pluginInstall);
        m_statbuf.st_uid = 321;
        m_mockDeviceUtil = std::make_shared<MockDeviceUtil>();
    }

    static constexpr int32_t EVENT_FD = 345;
    static constexpr pid_t DEFAULT_PID = 1999999999;

    static fanotify_event_metadata getMetaData(
        uint64_t _mask = FAN_CLOSE_WRITE,
        int32_t _fd = EVENT_FD,
        int32_t _pid = DEFAULT_PID,
        uint8_t _ver = FANOTIFY_METADATA_VERSION // Very unlikely to want to override this
        )
    {
        return {
            .event_len = FAN_EVENT_METADATA_LEN, .vers = _ver, .reserved = 0,
            .metadata_len = FAN_EVENT_METADATA_LEN, .mask = _mask, .fd = _fd, .pid = _pid
        };
    }

    static fanotify_event_metadata getOpenMetaData(int32_t _fd)
    {
        return getMetaData(FAN_OPEN, _fd);
    }

    void expectReadReturnsStruct(const fanotify_event_metadata& metadata)
    {
        EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
            .WillOnce(readReturnsStruct(metadata))
            .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));
    }

    void expectReadSameMetaDataTwice(const fanotify_event_metadata& metadata)
    {
        EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
            .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));
        EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
            .Times(2)
            .WillRepeatedly(readReturnsStruct(metadata))
            .RetiresOnSaturation();
    }

    void expectReadlinkReturnsPath(const char* filePath)
    {
        EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
            .WillOnce(readlinkReturnPath(filePath));
    }

    void defaultFstat()
    {
        EXPECT_CALL(*m_mockSysCallWrapper, fstat(_, _))
            .WillRepeatedly(fstatInodeEqualsFd());
    }

    void incrementingFstat()
    {
        EXPECT_CALL(*m_mockSysCallWrapper, fstat(_, _))
            .WillRepeatedly(fstatInodeIncrementing());
    }

    void defaultStat()
    {
        EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
            .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    }

    void expectStatProcPid()
    {
#ifdef USERNAME_UID_USED
        constexpr const char* path = "/proc/1999999999";
        EXPECT_CALL(*m_mockSysCallWrapper, _stat(StrEq(path), _)).WillOnce(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
#endif
    }

    void expectStatProcPid(int times)
    {
#ifdef USERNAME_UID_USED
        constexpr const char* path = "/proc/1999999999";
        EXPECT_CALL(*m_mockSysCallWrapper, _stat(StrEq(path), _))..Times(times).WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
#else
        std::ignore = times;
#endif
    }

    void setupExpectationsForOneEvent(const char* filePath);

    std::shared_ptr<EventReaderThread> makeDefaultEventReaderThread()
    {
        return std::make_shared<EventReaderThread>(m_fakeFanotify,
                                                   m_mockSysCallWrapper,
                                                   m_pluginInstall,
                                                   m_scanRequestQueue,
                                                   m_telemetryUtility,
                                                   m_mockDeviceUtil);
    }

    std::shared_ptr<EventReaderThread> makeSmallEventReaderThread(int logNotFull = 1, int readRepeatCount = 5)
    {
        auto ret =  std::make_shared<EventReaderThread>(m_fakeFanotify,
                                                   m_mockSysCallWrapper,
                                                   m_pluginInstall,
                                                   m_smallScanRequestQueue,
                                                   m_telemetryUtility,
                                                   m_mockDeviceUtil,
                                                   logNotFull);

        ret->readRepeatCount_ = readRepeatCount;
        return ret;
    }

    std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper;
    ScanRequestQueueSharedPtr m_scanRequestQueue;
    ScanRequestQueueSharedPtr m_smallScanRequestQueue;
    OnAccessTelemetryUtilitySharedPtr m_telemetryUtility;
    fs::path m_pluginInstall = "/opt/sophos-spl/plugins/av";
    std::shared_ptr<FakeFanotify> m_fakeFanotify;
    std::shared_ptr<MockDeviceUtil> m_mockDeviceUtil;
    struct ::stat m_statbuf {};
};

void TestEventReaderThread::setupExpectationsForOneEvent(const char* filePath)
{
    auto metadata = getMetaData();
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);
    expectReadlinkReturnsPath(filePath);
    expectStatProcPid();
    defaultFstat();
}

TEST_F(TestEventReaderThread, TestReaderExitsUsingPipe)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderLogsErrorIfFanotifySendsNoEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _)).WillOnce(Return(0));

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, ReceiveEvent)
{
    const char* filePath = "/tmp/test";
    setupExpectationsForOneEvent(filePath);

    {
        auto eventReader = makeDefaultEventReaderThread();
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    }

    ASSERT_EQ(m_scanRequestQueue->size(), 1);
    auto event = m_scanRequestQueue->pop();
    ASSERT_TRUE(event);
    EXPECT_EQ(event->getPath(), filePath);
    EXPECT_FALSE(event->isOpenEvent());
    EXPECT_FALSE(event->isCached());
}

TEST_F(TestEventReaderThread, ReceiveBadUnicode)
{
    const char* filePath = "/tmp/\xef\xbf\xbe_\xef\xbf\xbf_\xEF\xBF\xBE\xEF\xBF\xBF";
    setupExpectationsForOneEvent(filePath);

    {
        auto eventReader = makeDefaultEventReaderThread();
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    }

    ASSERT_EQ(m_scanRequestQueue->size(), 1);
    auto event = m_scanRequestQueue->pop();
    ASSERT_TRUE(event);
    EXPECT_EQ(event->getPath(), filePath);
}

TEST_F(TestEventReaderThread, ReceiveExcessivelyLongFilePath)
{
    const std::string filePath(5000, '_');
    setupExpectationsForOneEvent(filePath.data());
    /*
     * EventReader uses a buffer PATH_MAX size.
     * Per https://linux.die.net/man/2/readlink the system could return something bigger than PATH_MAX.
     */
    std::string_view expectedFilePath{filePath.data(), 4095};

    {
        auto eventReader = makeDefaultEventReaderThread();
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    }

    ASSERT_EQ(m_scanRequestQueue->size(), 1);
    auto event = m_scanRequestQueue->pop();
    ASSERT_TRUE(event);
    EXPECT_EQ(event->getPath(), expectedFilePath);
    EXPECT_FALSE(event->isOpenEvent());
}

TEST_F(TestEventReaderThread, TestReaderCanReceiveEventAfterNoEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(Return(0))
        .WillOnce(readReturnsStruct(metadata))
        .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));
    expectReadlinkReturnsPath(filePath);
    expectStatProcPid();
    defaultFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    ASSERT_EQ(m_scanRequestQueue->size(), 1);
    auto event = m_scanRequestQueue->pop();
    ASSERT_TRUE(event);
    EXPECT_EQ(event->getPath(), filePath);
    EXPECT_FALSE(event->isOpenEvent());
}


TEST_F(TestEventReaderThread, TestReaderReadsOnCloseFanotifyEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    expectReadlinkReturnsPath(filePath);
    expectStatProcPid();
    defaultFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderReadsCombinedFanotifyEventAndEventTypeSetAsClose)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto metadata = getMetaData(FAN_CLOSE_WRITE | FAN_OPEN);
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    expectReadlinkReturnsPath(filePath);
    expectStatProcPid();
    defaultFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream closeLogMsg;
    closeLogMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    closeLogMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(closeLogMsg.str(), 500ms));

    std::stringstream openLogMsg;
    openLogMsg << "On-open event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    openLogMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(openLogMsg.str()));

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    ASSERT_EQ(m_scanRequestQueue->size(), 1);
    ASSERT_EQ(m_scanRequestQueue->pop()->getScanType(), scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);
}


TEST_F(TestEventReaderThread, TestReaderReadsOnOpenFanotifyEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData(FAN_OPEN);
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    expectReadReturnsStruct(metadata);

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));
    expectStatProcPid();
    defaultFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-open event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderReadsOnOpenFanotifyEventAfterRestart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata1 = getMetaData(FAN_OPEN, 8000, 99999992);
    auto metadata2 = getMetaData(FAN_OPEN, 8001, 99999991);
    const char* filePath1 = "/tmp/test";
    const char* filePath2 = "/tmp/test/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(readReturnsStruct(metadata1))
        .WillOnce(readReturnsStruct(metadata2))
        .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillOnce(readlinkReturnPath(filePath1))
        .WillOnce(readlinkReturnPath(filePath2));
    expectStatProcPid(2);

    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8000, _))
        .WillOnce(fstatReturnsInode(1));
    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8001, _))
        .WillOnce(fstatReturnsInode(2));

    auto eventReader = makeDefaultEventReaderThread();
    eventReader->readRepeatCount_ = 1;
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg1;
    logMsg1 << "On-open event for " << filePath1 << " from Process (PID=" << metadata1.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg1 << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg1.str(), 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);

    eventReaderThread.requestStopIfNotStopped();

    clearMemoryAppender();
    eventReaderThread.startIfNotStarted();

    std::stringstream logMsg2;
    logMsg2 << "On-open event for " << filePath2 << " from Process (PID=" << metadata2.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg2 << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg2.str()));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 2);
}

TEST_F(TestEventReaderThread, TestReaderLogsUnexpectedFanotifyEventType)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData(FAN_ONDIR);
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectStatProcPid();
    expectReadReturnsStruct(metadata);
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "unknown operation mask: " << std::hex << metadata.mask;
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSetUnknownPathIfReadLinkFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    expectReadReturnsStruct(metadata);
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(Return(-1));
    expectStatProcPid();
    defaultFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for unknown from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    ASSERT_EQ(m_scanRequestQueue->size(), 1); // 0 will cause pop() to hang forever
    auto event = m_scanRequestQueue->pop();
    ASSERT_TRUE(event);
    EXPECT_EQ(event->getFd(), EVENT_FD);
    EXPECT_EQ(event->getPath(), "unknown");
}

#ifdef USERNAME_UID_USED
TEST_F(TestEventReaderThread, TestReaderSetInvalidUidIfStatFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _)).WillOnce(Return(-1));
    defaultFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ")";
    logMsg << " and UID unknown";
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}
#endif

TEST_F(TestEventReaderThread, TestReaderExitsIfFanotifyProtocolVersionIsTooOld)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    int version = 2;

    auto metadata = getMetaData(FAN_CLOSE, 10, 10, version);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));
    expectReadReturnsStruct(metadata);

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "Fanotify wrong protocol version " << version;
    EXPECT_TRUE(waitForLog(logMsg.str()));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsWithoutFD)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData(FAN_CLOSE, -1);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Got fanotify metadata event without fd"));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsWithSoapdPid)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData(FAN_CLOSE,  EVENT_FD, getpid());

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderSkipsEventsInPluginLogDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/opt/sophos-spl/plugins/av/log/soapd.log";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events", 500ms));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderDoesntSkipEventsInSimilarDirToPluginLogDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/opt/sophos-spl/plugins/av/logz/soapd.log";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));
    expectStatProcPid();
    defaultFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events", 500ms));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, TestReaderSkipsExcludedPaths)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    std::vector<common::Exclusion> exclusions;
    exclusions.emplace_back("/excluded/");
    auto metadata = getMetaData();

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);
    const char* filePath = "/excluded/eicar.com";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    eventReader->setExclusions(exclusions);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events", 500ms));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderDoesntInvalidateFd)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _)).WillOnce(readlinkReturnPath(filePath));
    expectStatProcPid();
    defaultFstat();


    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events", 500ms));
    eventReaderThread.requestStopIfNotStopped();

    ASSERT_EQ(m_scanRequestQueue->size(), 1);
    auto popItem = m_scanRequestQueue->pop();
    EXPECT_TRUE(popItem != nullptr);
    EXPECT_NE(popItem->getFd(), -1);
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

TEST_F(TestEventReaderThread, TestReaderLogsFanotifyQueueOverflow)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto metadata = getMetaData(FAN_Q_OVERFLOW, FAN_NOFD);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    expectReadReturnsStruct(metadata);

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Fanotify queue overflowed, some files will not be scanned.", 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 0);
}

// DeDup tests

TEST_F(TestEventReaderThread, EventsWithSameInodeAndDeviceAreSkipped)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto metadata = getOpenMetaData(8000);
    expectReadSameMetaDataTwice(metadata);
    defaultStat();

    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8000, _))
        .Times(2)
        .WillRepeatedly(fstatReturnsDeviceAndInode(1,1));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .Times(2)
        .WillRepeatedly(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    {
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    } // Wait for Reader to finish
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, EventsWithSameInodeAndDeviceButDifferentPathAreSkipped)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto metadata1 = getOpenMetaData(8000);
    auto metadata2 = getOpenMetaData(8001);
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(readReturnsStruct(metadata1))
        .WillOnce(readReturnsStruct(metadata2))
        .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));

    defaultStat();

    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8000, _))
        .WillOnce(fstatReturnsDeviceAndInode(1,1));
    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8001, _))
        .WillOnce(fstatReturnsDeviceAndInode(1,1));

    const char* filePath1 = "/tmp/test";
    const char* filePath2 = "/tmp2/test2";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(StrEq("/proc/self/fd/8000"), _, _))
        .WillOnce(readlinkReturnPath(filePath1));
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(StrEq("/proc/self/fd/8001"), _, _))
        .WillOnce(readlinkReturnPath(filePath2));

    auto eventReader = makeDefaultEventReaderThread();
    {
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    } // Wait for Reader to finish
    EXPECT_EQ(m_scanRequestQueue->size(), 1);
}

TEST_F(TestEventReaderThread, EventsWithDifferentDeviceAreNotSkipped)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto metadata = getOpenMetaData(8000);
    expectReadSameMetaDataTwice(metadata);

    defaultStat();

    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8000, _))
        .WillOnce(fstatReturnsDeviceAndInode(2,1))
        .WillOnce(fstatReturnsDeviceAndInode(1,1));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .Times(2)
        .WillRepeatedly(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    {
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    } // Wait for Reader to finish
    EXPECT_EQ(m_scanRequestQueue->size(), 2);
}

TEST_F(TestEventReaderThread, EventsWithHighBitDifferentDeviceAreNotSkipped)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto metadata = getOpenMetaData(8000);
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .Times(2)
        .WillRepeatedly(readReturnsStruct(metadata))
        .RetiresOnSaturation();

    defaultStat();

    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8000, _))
        .WillOnce(fstatReturnsDeviceAndInode(0x0000000000000001,0x0000000000000001))
        .WillOnce(fstatReturnsDeviceAndInode(0x8000000000000001,0x0000000000000001));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .Times(2)
        .WillRepeatedly(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    {
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    } // Wait for Reader to finish
    EXPECT_EQ(m_scanRequestQueue->size(), 2);
}

TEST_F(TestEventReaderThread, EventsWithDifferentInodeAreNotSkipped)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto metadata = getOpenMetaData(8000);
    expectReadSameMetaDataTwice(metadata);
    defaultStat();

    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8000, _))
        .WillOnce(fstatReturnsDeviceAndInode(1,1))
        .WillOnce(fstatReturnsDeviceAndInode(1,2));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .Times(2)
        .WillRepeatedly(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    {
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    } // Wait for Reader to finish
    EXPECT_EQ(m_scanRequestQueue->size(), 2);
}

TEST_F(TestEventReaderThread, EventsWithHighBitDifferentInodeAreNotSkipped)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    auto metadata = getOpenMetaData(8000);
    expectReadSameMetaDataTwice(metadata);
    defaultStat();

    EXPECT_CALL(*m_mockSysCallWrapper, fstat(8000, _))
        .WillOnce(fstatReturnsDeviceAndInode(0x0000000000000001,0x0000000000000001))
        .WillOnce(fstatReturnsDeviceAndInode(0x0000000000000001,0x8000000000000001));

    const char* filePath = "/tmp/test";
    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .Times(2)
        .WillRepeatedly(readlinkReturnPath(filePath));

    auto eventReader = makeDefaultEventReaderThread();
    {
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    } // Wait for Reader to finish
    EXPECT_EQ(m_scanRequestQueue->size(), 2);
}

// Queue Full events

TEST_F(TestEventReaderThread, TestReaderLogsQueueIsFull)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));

    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full.", 500ms));
    EXPECT_EQ(m_smallScanRequestQueue->size(), 3);
}

TEST_F(TestEventReaderThread, TestReaderDoesntLogQueueIsFullWhenIsAlreadyFull)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    ASSERT_TRUE(waitForLog("Stopping the reading of Fanotify events", 500ms));
    EXPECT_TRUE(appenderContainsCount("Failed to add scan request to queue, on-access scanning queue is full.", 1));
    EXPECT_EQ(m_smallScanRequestQueue->size(), 3);
}

TEST_F(TestEventReaderThread, TestReaderLogsQueueIsFullWhenItFillsSecondTime)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent eventReaderGuard;

    auto metadata = getMetaData();
    constexpr const char* filePath = "/tmp/test";

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

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread(1, 1);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full.", 500ms));
    m_smallScanRequestQueue->restart();

    clearMemoryAppender();
    eventReaderGuard.onEventNoArgs();
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    EXPECT_TRUE(appenderContains("Queue is no longer full. Number of events dropped: 1"));
}

TEST_F(TestEventReaderThread, TestReaderDoesntLogWhenQueueSizeHasntLoweredToLogThreshold)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent queueFull, queueNotFull;

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&queueFull, &WaitForEvent::onEventNoArgs),
            InvokeWithoutArgs(&queueNotFull, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(1, POLLIN)))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread(2, 1);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    queueFull.waitDefault();
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    EXPECT_TRUE(waitForLogMultiple("On-close event for /tmp/test from Process (PID=1999999999)", 5, 100ms));
    m_smallScanRequestQueue->pop();
    EXPECT_EQ(m_smallScanRequestQueue->size(), 2);

    clearMemoryAppender();
    queueNotFull.onEventNoArgs();
    EXPECT_TRUE(waitForLogMultiple("On-close event for /tmp/test from Process (PID=1999999999)", 2, 100ms));
    EXPECT_EQ(m_smallScanRequestQueue->size(), 3);
    EXPECT_FALSE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    EXPECT_FALSE(appenderContains("Queue is no longer full. Number of events dropped: 2"));
}


TEST_F(TestEventReaderThread, TestReaderLogsWhenQueueSizeHasLoweredToLogThreshold)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent queueFull, queueNotFull;

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&queueFull, &WaitForEvent::onEventNoArgs),
            InvokeWithoutArgs(&queueNotFull, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(1, POLLIN)))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread(2, 1);
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    queueFull.waitDefault();
    EXPECT_TRUE(waitForLogMultiple("On-close event for /tmp/test from Process (PID=1999999999)", 4, 100ms));
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    ASSERT_EQ(m_smallScanRequestQueue->size(), 3);

    m_smallScanRequestQueue->pop();
    m_smallScanRequestQueue->pop();
    m_smallScanRequestQueue->pop();

    clearMemoryAppender();
    queueNotFull.onEventNoArgs();
    EXPECT_TRUE(waitForLog("Queue is no longer full. Number of events dropped: 1"));
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
}

TEST_F(TestEventReaderThread, TestReaderLogsManyEventsMissed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent queueFull, queueEmpty;

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(DoAll(
            InvokeWithoutArgs(&queueFull, &WaitForEvent::onEventNoArgs),
            InvokeWithoutArgs(&queueEmpty, &WaitForEvent::waitDefault),
            pollReturnsWithRevents(1, POLLIN)))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    queueFull.waitDefault();
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    m_smallScanRequestQueue->restart();

    queueEmpty.onEventNoArgs();
    EXPECT_TRUE(waitForLog("Queue is no longer full. Number of events dropped: 2", 100ms));
}

TEST_F(TestEventReaderThread, TestReaderResetsMissedEventCountAfterStopStart)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    WaitForEvent eventReaderGuard;

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

    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));

    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    //Wait for queue to fill before restarting
    eventReaderGuard.onEventNoArgs();
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full.", 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));

    //Restart
    eventReaderThread.requestStopIfNotStopped();
    eventReaderGuard.clear();

    clearMemoryAppender();
    m_smallScanRequestQueue->restart();
    eventReaderThread.startIfNotStarted();

    //Wait for queue to fill again and make critical observation
    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full."));
    EXPECT_FALSE(appenderContains("Queue is no longer full. Number of events dropped: 1"));

    //End test
    eventReaderGuard.onEventNoArgs();
}

TEST_F(TestEventReaderThread, TestReaderLogsErrorFromPpollAndThrows)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(SetErrnoAndReturn(EBADF, -1));

    auto eventReader = makeDefaultEventReaderThread();

    try
    {
        eventReader->innerRun();
        FAIL() << "EventReaderThread::innerRun() didn't throw";
    }
    catch(const std::runtime_error& e)
    {
        EXPECT_STREQ(e.what(), "Error from innerRun poll: 9 (Bad file descriptor)");
    }
}

TEST_F(TestEventReaderThread, TestReaderContinuesQuietyWhenPpollReturnsEINTR)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(SetErrnoAndReturn(EINTR, -1))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    expectReadReturnsStruct(metadata);
    expectReadlinkReturnsPath(filePath);
    expectStatProcPid();
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));
    EXPECT_FALSE(appenderContains("Error from poll"));
}

/*
 * errno for read are divided into 4 groups
 * * return
 * * LOGWARN and return (count to 20) exception
 * * LOGERROR, sleep and return (count to 20) exception
 * * exception
 */

class TestWithType1Errno : public TestEventReaderThread,
                      public testing::WithParamInterface<int>
{};

TEST_P(TestWithType1Errno, readEventAfterReadErrno)
{
    int errnoValue = GetParam();
    LOG("Testing errno=" << errnoValue);
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(SetErrnoAndReturn(errnoValue, -1))
        .WillOnce(readReturnsStruct(metadata))
        .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));
    expectReadlinkReturnsPath(filePath);
    expectStatProcPid();
    incrementingFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);

    EXPECT_FALSE(appenderContains("Failed to read fanotify event"));
}

INSTANTIATE_TEST_SUITE_P(TestReaderCanReceiveEventAfterRetryErrno,
                         TestWithType1Errno,
                         testing::Values(EINTR, EAGAIN)
                         );

class TestWithType2Errno : public TestEventReaderThread,
                           public testing::WithParamInterface<int>
{};

TEST_P(TestWithType2Errno, readEventAfterReadErrno)
{
    int errnoValue = GetParam();
    LOG("Testing errno=" << errnoValue);
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(SetErrnoAndReturn(errnoValue, -1))
        .WillOnce(readReturnsStruct(metadata))
        .WillRepeatedly(SetErrnoAndReturn(EAGAIN, -1));
    expectReadlinkReturnsPath(filePath);
    expectStatProcPid();
    incrementingFstat();

    auto eventReader = makeDefaultEventReaderThread();
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    std::stringstream logMsg;
    logMsg << "On-close event for " << filePath << " from Process (PID=" << metadata.pid << ")";
#ifdef USERNAME_UID_USED
    logMsg << " and UID " << m_statbuf.st_uid;
#endif
    EXPECT_TRUE(waitForLog(logMsg.str(), 500ms));

    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));
    EXPECT_EQ(m_scanRequestQueue->size(), 1);

    EXPECT_TRUE(appenderContains("Failed to read fanotify event")); // Logs warning or error
}

TEST_P(TestWithType2Errno, diesAfterHittingLimit)
{
    const int errnoValue = GetParam();
    LOG("Testing errno=" << errnoValue);

    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillRepeatedly(pollReturnsWithRevents(1, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .Times(EventReaderThread::RESTART_SOAP_ERROR_COUNT+1)
        .WillRepeatedly(SetErrnoAndReturn(errnoValue, -1));

    auto eventReader = makeDefaultEventReaderThread();
    eventReader->m_out_of_file_descriptor_delay = std::chrono::milliseconds{1};
    try
    {
        eventReader->innerRun();
        FAIL() << "EventReaderThread::diesAfterHittingLimit() didn't throw";
    }
    catch(const std::runtime_error& e)
    {
        switch (errnoValue)
        {
            case EPERM:
                EXPECT_STREQ(e.what(), "Too many EPERM/EACCES/ENOENT errors. Restarting On Access: (1 Operation not permitted)");
                break;
            case ENOENT:
                EXPECT_STREQ(e.what(), "Too many EPERM/EACCES/ENOENT errors. Restarting On Access: (2 No such file or directory)");
                break;
            case EACCES:
                EXPECT_STREQ(e.what(), "Too many EPERM/EACCES/ENOENT errors. Restarting On Access: (13 Permission denied)");
                break;
            case ENFILE:
            case EMFILE:
                EXPECT_STREQ(e.what(), "No more File Descriptors available. Restarting On Access");
                break;
            default:
                FAIL() << "Handling unknown errno: " << e.what();
                break;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(TestReaderCanReceiveEventAfterRetryErrno,
                         TestWithType2Errno,
                         testing::Values(EPERM, ENOENT, EACCES, ENFILE, EMFILE)
);

namespace
{
    class TestWithType4Errno : public TestEventReaderThread, public testing::WithParamInterface<int>
    {
    };
}

TEST_P(TestWithType4Errno, NotRecoverable)
{
    const int errnoValue = GetParam();
    LOG("Testing errno=" << errnoValue);
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillOnce(SetErrnoAndReturn(errnoValue, -1));

    auto eventReader = makeDefaultEventReaderThread();
    try
    {
        eventReader->innerRun();
        FAIL() << "EventReaderThread::throwIfErrorNotRecoverable() didn't throw";
    }
    catch(const std::runtime_error& e)
    {
        switch (errnoValue)
        {
            case EIO:
                EXPECT_STREQ(e.what(), "Fatal Error. Restarting On Access: (5 Input/output error)");
                break;
            case EBADF:
                EXPECT_STREQ(e.what(), "Fatal Error. Restarting On Access: (9 Bad file descriptor)");
                break;
            case EINVAL:
                EXPECT_STREQ(e.what(), "Fatal Error. Restarting On Access: (22 Invalid argument)");
                break;
            case ETXTBSY:
                EXPECT_STREQ(e.what(), "Fatal Error. Restarting On Access: (26 Text file busy)");
                break;
            default:
                FAIL() << "Handling unknown errno: " << e.what();
                break;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(TestReaderThrowsWhenErrorNotRecoverable,
                         TestWithType4Errno,
                         testing::Values(EIO, EBADF, EINVAL, ETXTBSY)
);

TEST_F(TestEventReaderThread, TestReaderIncrementsTelemetryOnEventDropped)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto metadata = getMetaData();
    const char* filePath = "/tmp/test";

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, 2, _, nullptr))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));
    EXPECT_CALL(*m_mockSysCallWrapper, read(FANOTIFY_FD, _, _))
        .WillRepeatedly(readReturnsStruct(metadata));

    EXPECT_CALL(*m_mockSysCallWrapper, readlink(_, _, _))
        .WillRepeatedly(readlinkReturnPath(filePath));
    EXPECT_CALL(*m_mockSysCallWrapper, _stat(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(m_statbuf), Return(0)));
    incrementingFstat();

    auto eventReader = makeSmallEventReaderThread();
    eventReader->readRepeatCount_ = 1;
    common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);

    EXPECT_TRUE(waitForLog("Failed to add scan request to queue, on-access scanning queue is full.", 500ms));
    EXPECT_TRUE(waitForLog("Stopping the reading of Fanotify events"));

    auto telemetry = m_telemetryUtility->getTelemetry();
    EXPECT_EQ(telemetry.m_percentageEventsDropped, 50.0f);
}

TEST_F(TestEventReaderThread, setCacheAll)
{
    auto eventReader = makeSmallEventReaderThread();
    EXPECT_NO_THROW(eventReader->setCacheAllEvents(true));
    EXPECT_NO_THROW(eventReader->setCacheAllEvents(false));
}

TEST_F(TestEventReaderThread, CacheEvent)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    getLogger().setLogLevel(log4cplus::TRACE_LOG_LEVEL);
    const char* filePath = "/tmp/test";
    setupExpectationsForOneEvent(filePath);
    EXPECT_CALL(*m_mockDeviceUtil, isCachable(EVENT_FD)).WillOnce(Return(true));
    auto fakeFanotify = std::make_shared<MockFanotify>();
    m_fakeFanotify = fakeFanotify;
    EXPECT_CALL(*fakeFanotify, cacheFd(EVENT_FD,filePath,false)).Times(1);

    {
        auto eventReader = makeDefaultEventReaderThread();
        EXPECT_NO_THROW(eventReader->setCacheAllEvents(true));
        common::ThreadRunner eventReaderThread(eventReader, "eventReader", true);
    }
    ASSERT_EQ(m_scanRequestQueue->size(), 1);
    auto event = m_scanRequestQueue->pop();
    ASSERT_TRUE(event);
    EXPECT_TRUE(event->isCached());
    EXPECT_TRUE(waitForLog("Cached /tmp/test from Process (PID=1999999999)"));
}
