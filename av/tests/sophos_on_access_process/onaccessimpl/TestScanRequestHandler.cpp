// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "OnAccessImplMemoryAppenderUsingTests.h"

#include "common/ThreadRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/local_settings/OnAccessProductConfigDefaults.h"
#include "sophos_on_access_process/onaccessimpl/ReconnectSettings.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"

#include "tests/common/RecordingMockSocket.h"
#include "tests/mount_monitor/mountinfoimpl/MockDeviceUtil.h"
#include "tests/sophos_on_access_process/fanotifyhandler/MockFanotifyHandler.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

namespace fs = sophos_filesystem;
namespace fanotifyhandler = sophos_on_access_process::fanotifyhandler;
using namespace ::testing;
using namespace sophos_on_access_process::onaccessimpl;
using namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry;
using namespace sophos_on_access_process::local_settings;

using ScanRequest = ScanRequestHandler::scan_request_t;
using ScanRequestPtr = ScanRequestHandler::scan_request_ptr_t;

namespace
{
    constexpr auto DEFAULT_QUEUE_SIZE = defaultMaxScanQueueSize;

    class TestScanRequestHandler : public OnAccessImplMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir / "var");

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
            appConfig.setData("PLUGIN_INSTALL", m_testDir );

            m_mockFanotifyHandler = std::make_shared<NiceMock<MockFanotifyHandler>>();
            m_mockDeviceUtil = std::make_shared<MockDeviceUtil>();
            m_telemetryUtility = std::make_shared<OnAccessTelemetryUtility>();
        }

        using HandlerPtr = std::shared_ptr<sophos_on_access_process::onaccessimpl::ScanRequestHandler>;

        HandlerPtr buildDefaultHandler(std::shared_ptr<unixsocket::IScanningClientSocket> socket);
        [[maybe_unused]] HandlerPtr buildDefaultHandler();

        static ScanRequestPtr emptyRequest()
        {
            return std::make_shared<ScanRequest>();
        }

        static ScanRequestPtr buildRequest(
            scan_messages::E_SCAN_TYPE type = scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN,
            const char* path = "/expected"
            )
        {
            auto request = emptyRequest();
            request->setScanType(type);
            request->setPath(path);
            return request;
        }

        std::shared_ptr<MockFanotifyHandler> m_mockFanotifyHandler;
        std::shared_ptr<MockDeviceUtil> m_mockDeviceUtil;
        OnAccessTelemetryUtilitySharedPtr m_telemetryUtility;
        fs::path m_testDir;
    };

    constexpr struct timespec oneMillisecond { 0, 1000000 }; // 1ms
}

TestScanRequestHandler::HandlerPtr TestScanRequestHandler::buildDefaultHandler(std::shared_ptr<unixsocket::IScanningClientSocket> socket)
{
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>(DEFAULT_QUEUE_SIZE);
    return std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, std::move(socket), m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
}

[[maybe_unused]] TestScanRequestHandler::HandlerPtr TestScanRequestHandler::buildDefaultHandler()
{
    auto socket = std::make_shared<RecordingMockSocket>();
    return buildDefaultHandler(socket);
}

TEST_F(TestScanRequestHandler, scan_fileDetected)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath = "/tmp/test";
    auto scanRequest = emptyRequest();
    scanRequest->setPath(filePath);
    scanRequest->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);

    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        nullptr, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
    scanHandler->scan(scanRequest);

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), filePath);
    std::stringstream logMsg;
    logMsg << "ScanRequestHandler-1 detected \"" << filePath << "\" is infected with threatName (Close-Write)";
    EXPECT_TRUE(appenderContains(logMsg.str()));
}

TEST_F(TestScanRequestHandler, scan_error)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath = "/tmp/test";
    auto scanRequest = emptyRequest();
    scanRequest->setPath(filePath);
    scanRequest->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);

    auto socket = std::make_shared<ExceptionThrowingTestSocket>(false);
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        nullptr, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
    scanHandler->scan(scanRequest, oneMillisecond);

    std::stringstream logMsg;
    logMsg << "ScanRequestHandler-1 received error: Failed to scan file: " << filePath << " after " << MAX_SCAN_RETRIES << " retries";
    EXPECT_TRUE(appenderContains(logMsg.str()));
}

TEST_F(TestScanRequestHandler, scan_abortUncachesFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::vector<ScanRequestPtr> scanRequests;
    for (uint i = 1; i <= 5; i++)
    {
        std::string filePath = "/tmp/test" + std::to_string(i);
        auto scanRequest = buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE, filePath.c_str());
        scanRequests.push_back(std::move(scanRequest));
    }

    auto socket = std::make_shared<ExceptionThrowingTestSocket>(false);
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
            nullptr, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);

    for (auto const& scanRequest : scanRequests)
    {
        scanHandler->scan(scanRequest, oneMillisecond);
    }

    std::stringstream logMsg;
    logMsg << "Reached total maximum number of reconnection attempts. Aborting scan of " << scanRequests.back()->getPath();
    EXPECT_TRUE(appenderContains(logMsg.str()));
    EXPECT_TRUE(appenderContains("un-caching /tmp/test5 (Close-Write)"));
}

TEST_F(TestScanRequestHandler, scan_errorWhenShuttingDown)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath = "/tmp/test";
    auto scanRequest = emptyRequest();
    scanRequest->setPath(filePath);

    auto socket = std::make_shared<ExceptionThrowingTestSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        nullptr, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
    EXPECT_THROW(scanHandler->scan(scanRequest), ScanInterruptedException);

    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 scan aborted: Scanner received stop notification"));
}

TEST_F(TestScanRequestHandler, scan_threadPopsAllItemsFromQueue)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath1 = "/tmp/test1";
    const char* filePath2 = "/tmp/test2";
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>(DEFAULT_QUEUE_SIZE);
    auto scanRequest1 = emptyRequest();
    scanRequest1->setPath(filePath1);
    scanRequest1->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN);
    scanRequest1->setFd(123);
    auto scanRequest2 = emptyRequest();
    scanRequest2->setPath(filePath2);
    scanRequest2->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);
    scanRequest2->setFd(123);
    scanRequestQueue->emplace(scanRequest1);
    scanRequestQueue->emplace(scanRequest2);

    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);

    std::stringstream logMsg1;
    logMsg1 << "ScanRequestHandler-1 detected \"" << filePath1 << "\" is infected with threatName (Open)";
    std::stringstream logMsg2;
    logMsg2 << "ScanRequestHandler-1 detected \"" << filePath2 << "\" is infected with threatName (Close-Write)";
    EXPECT_TRUE(waitForLog(logMsg1.str(), 500ms));
    EXPECT_TRUE(waitForLog(logMsg2.str()));
    ASSERT_EQ(socket->m_paths.size(), 2);
    scanRequestQueue->stop();
}

TEST_F(TestScanRequestHandler, scan_LogsWhenPopsRequestFromQueue)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    getLogger().setLogLevel(log4cplus::TRACE_LOG_LEVEL);

    const char* filePath1 = "/tmp/test1";
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>(DEFAULT_QUEUE_SIZE);
    auto scanRequest1 = emptyRequest();
    scanRequest1->setPath(filePath1);
    scanRequest1->setUserID("2345");
    scanRequestQueue->emplace(scanRequest1);

    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);

    EXPECT_TRUE(waitForLog("ScanRequestHandler-1 picked up scan request for /tmp/test1 with UID 2345", 500ms));

    scanRequestQueue->stop();
}

TEST_F(TestScanRequestHandler, scan_threadCanExitWhileWaiting)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scanRequestQueue = std::make_shared<ScanRequestQueue>(DEFAULT_QUEUE_SIZE);
    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);
    EXPECT_TRUE(waitForLog("Starting ScanRequestHandler-1"));
    scanRequestQueue->stop();
    scanHandlerThread->requestStopIfNotStopped();
    EXPECT_TRUE(waitForLog("Finished ScanRequestHandler-1"));
}

TEST_F(TestScanRequestHandler, scan_threadCanExitWhileScanning)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scanRequestQueue = std::make_shared<ScanRequestQueue>(DEFAULT_QUEUE_SIZE);
    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);

    EXPECT_TRUE(waitForLog("Starting ScanRequestHandler-1"));

    auto scanRequest = emptyRequest();
    scanRequest->setPath("THROW_SCAN_INTERRUPTED");
    scanRequestQueue->emplace(scanRequest);

    EXPECT_TRUE(waitForLog("Finished ScanRequestHandler-1"));
}

TEST_F(TestScanRequestHandler, cleanScanOpen)
{
    auto socket = std::make_shared<RecordingMockSocket>(false, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockDeviceUtil, isCachable(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(1);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).Times(0);

    ScanRequestPtr request(buildRequest());
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
}

TEST_F(TestScanRequestHandler, cleanScanClose)
{
    auto socket = std::make_shared<RecordingMockSocket>(false, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockDeviceUtil, isCachable(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(1);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).Times(0);

    ScanRequestPtr request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
}

TEST_F(TestScanRequestHandler, cleanScanOpenRemote)
{
    auto socket = std::make_shared<RecordingMockSocket>(false, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockDeviceUtil, isCachable(_)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).Times(0);

    ScanRequestPtr request(buildRequest());
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
}

TEST_F(TestScanRequestHandler, cleanScanCloseRemote)
{
    auto socket = std::make_shared<RecordingMockSocket>(false, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockDeviceUtil, isCachable(_)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).Times(0);

    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
}


TEST_F(TestScanRequestHandler, infectedScanOpen)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(true, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).Times(1);

    auto request(buildRequest());
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 detected \"/expected\" is infected with threatName (Open)"));
}

TEST_F(TestScanRequestHandler, infectedScanClose)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(true, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).WillOnce(Return(true));

    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 detected \"/expected\" is infected with threatName (Close-Write)"));
}

TEST_F(TestScanRequestHandler, infectedScanCloseIgnoreUncacheFailure)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(true, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).WillOnce(Return(false));

    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 detected \"/expected\" is infected with threatName (Close-Write)"));
}

TEST_F(TestScanRequestHandler, socketScanThrowsScanInterrupted)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(false, true);
    auto scanHandler = buildDefaultHandler(socket);
    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    request->setPath("THROW_SCAN_INTERRUPTED");
    EXPECT_ANY_THROW(scanHandler->scan(request));
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 scan aborted: THROW_SCAN_INTERRUPTED"));
}

TEST_F(TestScanRequestHandler, socketScanThrowsStdException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(false, true);
    auto scanHandler = buildDefaultHandler(socket);
    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    request->setPath("THROW_BAD_ALLOC");
    scanHandler->scan(request);
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 failed to scan THROW_BAD_ALLOC : std::bad_alloc"));
}

TEST_F(TestScanRequestHandler, scanError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(false, true);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).Times(1);

    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 received error: Scan Error"));
}

// Follow naming convention https://github.com/google/googletest/blob/main/docs/advanced.md#death-test-naming
using TestScanRequestHandlerDeathTest = TestScanRequestHandler;

TEST_F(TestScanRequestHandlerDeathTest, cacheFdError)
{
    //starts test in a new child process and tries to ensure safety of the main test process
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    testing::Mock::AllowLeak(m_mockFanotifyHandler.get());
    testing::Mock::AllowLeak(m_mockDeviceUtil.get());

    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(false, false);
    auto scanHandler = buildDefaultHandler(socket);

    //WillRepeatedly allows us to set the return code and gmock doesn't have to verify how many times we called cacheFd
    EXPECT_CALL(*m_mockDeviceUtil, isCachable(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).WillRepeatedly(Return(-1));

    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN));
    EXPECT_EXIT(scanHandler->scan(request), ::testing::ExitedWithCode(1), "");
}

TEST_F(TestScanRequestHandler, uncacheFdErrorIgnored)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(true, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).WillOnce(Return(-1));

    auto request(buildRequest());
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 detected \"/expected\" is infected with threatName (Open)"));
}

TEST_F(TestScanRequestHandler, scanErrorAndDetection)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(true, true);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_,_)).Times(0);
    EXPECT_CALL(*m_mockFanotifyHandler, uncacheFd(_,_)).Times(1);

    auto request(buildRequest(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE));
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 received error: Scan Error"));
    EXPECT_TRUE(appenderContains("ScanRequestHandler-1 detected \"/expected\" is infected with threatName (Close-Write)"));
}

TEST_F(TestScanRequestHandler, scan_threadCanDumpPerfData)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath1 = "/tmp/test1";
    const char* filePath2 = "/tmp/test2";
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>(DEFAULT_QUEUE_SIZE);
    auto scanRequest1 = emptyRequest();
    scanRequest1->setPath(filePath1);
    scanRequest1->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN);
    scanRequest1->setFd(123);
    auto scanRequest2 = emptyRequest();
    scanRequest2->setPath(filePath2);
    scanRequest2->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);
    scanRequest2->setFd(123);
    scanRequestQueue->emplace(scanRequest1);
    scanRequestQueue->emplace(scanRequest2);

    auto socket = std::make_shared<RecordingMockSocket>();
    sophos_on_access_process::local_settings::OnAccessLocalSettings localSettings;
    localSettings.dumpPerfData = true;
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket, m_mockFanotifyHandler, m_mockDeviceUtil, m_telemetryUtility, 123, localSettings);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);

    std::stringstream logMsg1;
    logMsg1 << "ScanRequestHandler-123 detected \"" << filePath1 << "\" is infected with threatName (Open)";
    std::stringstream logMsg2;
    logMsg2 << "ScanRequestHandler-123 detected \"" << filePath2 << "\" is infected with threatName (Close-Write)";
    EXPECT_TRUE(waitForLog(logMsg1.str()));
    EXPECT_TRUE(waitForLog(logMsg2.str()));
    ASSERT_EQ(socket->m_paths.size(), 2);
    scanRequestQueue->stop();

    ASSERT_FALSE(fs::is_empty(m_testDir / "var"));
    for (const auto& perfDumpFile : fs::directory_iterator(m_testDir / "var"))
    {
        std::ifstream perfDumpFH(perfDumpFile.path());
        std::string line;
        getline(perfDumpFH, line);
        EXPECT_EQ(line, "Scan duration\tIn-Product duration\tQueue size\tFile path");
        getline(perfDumpFH, line);
        EXPECT_TRUE(line.find(filePath1) != std::string::npos);
        getline(perfDumpFH, line);
        EXPECT_TRUE(line.find(filePath2) != std::string::npos);
    }
}
