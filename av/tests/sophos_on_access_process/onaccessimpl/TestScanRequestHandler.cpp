// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessImplMemoryAppenderUsingTests.h"

#include "common/RecordingMockSocket.h"
#include "common/ThreadRunner.h"

#include "sophos_on_access_process/onaccessimpl/ReconnectSettings.h"
#include "sophos_on_access_process/fanotifyhandler/MockFanotifyHandler.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"

#include <gtest/gtest.h>

#include <sstream>

namespace fanotifyhandler = sophos_on_access_process::fanotifyhandler;
using namespace ::testing;
using namespace sophos_on_access_process::onaccessimpl;

namespace
{
    class TestScanRequestHandler : public OnAccessImplMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            m_mockFanotifyHandler = std::make_shared<NiceMock<MockFanotifyHandler>>();
        }

        using HandlerPtr = std::shared_ptr<sophos_on_access_process::onaccessimpl::ScanRequestHandler>;

        HandlerPtr buildDefaultHandler(std::shared_ptr<unixsocket::IScanningClientSocket> socket);
        [[maybe_unused]] HandlerPtr buildDefaultHandler();

        scan_messages::ClientScanRequestPtr buildRequest()
        {
            scan_messages::ClientScanRequestPtr request = std::make_shared<scan_messages::ClientScanRequest>();
            request->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN);
            request->setPath("/expected");
            return request;
        }

        std::shared_ptr<NiceMock<MockFanotifyHandler>> m_mockFanotifyHandler;
    };

    const struct timespec& oneMillisecond = { 0, 1000000 }; // 1ms
}

TestScanRequestHandler::HandlerPtr TestScanRequestHandler::buildDefaultHandler(std::shared_ptr<unixsocket::IScanningClientSocket> socket)
{
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>();
    return std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, std::move(socket), m_mockFanotifyHandler);
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
    auto scanRequest = std::make_shared<ClientScanRequest>();
    scanRequest->setPath(filePath);
    scanRequest->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);

    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        nullptr, socket, m_mockFanotifyHandler);
    scanHandler->scan(scanRequest);

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), filePath);
    std::stringstream logMsg;
    logMsg << "Detected \"" << filePath << "\" is infected with threatName (Close-Write)";
    EXPECT_TRUE(appenderContains(logMsg.str()));
}

TEST_F(TestScanRequestHandler, scan_error)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath = "/tmp/test";
    auto scanRequest = std::make_shared<ClientScanRequest>();
    scanRequest->setPath(filePath);
    scanRequest->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);

    auto socket = std::make_shared<ExceptionThrowingTestSocket>(false);
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        nullptr, socket, m_mockFanotifyHandler);
    scanHandler->scan(scanRequest, oneMillisecond);

    std::stringstream logMsg;
    logMsg << "Failed to scan file: " << filePath << " after " << MAX_SCAN_RETRIES << " retries";
    EXPECT_TRUE(appenderContains(logMsg.str()));
}

TEST_F(TestScanRequestHandler, scan_errorWhenShuttingDown)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath = "/tmp/test";
    auto scanRequest = std::make_shared<ClientScanRequest>();
    scanRequest->setPath(filePath);

    auto socket = std::make_shared<ExceptionThrowingTestSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        nullptr, socket, m_mockFanotifyHandler);
    scanHandler->scan(scanRequest);

    EXPECT_TRUE(appenderContains("Scan aborted: Scanner received stop notification"));
}

TEST_F(TestScanRequestHandler, scan_threadPopsAllItemsFromQueue)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath1 = "/tmp/test1";
    const char* filePath2 = "/tmp/test2";
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>();
    auto scanRequest1 = std::make_shared<ClientScanRequest>();
    scanRequest1->setPath(filePath1);
    scanRequest1->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_OPEN);
    scanRequest1->setFd(123);
    auto scanRequest2 = std::make_shared<ClientScanRequest>();
    scanRequest2->setPath(filePath2);
    scanRequest2->setScanType(scan_messages::E_SCAN_TYPE_ON_ACCESS_CLOSE);
    scanRequest2->setFd(123);
    scanRequestQueue->emplace(scanRequest1);
    scanRequestQueue->emplace(scanRequest2);

    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket, m_mockFanotifyHandler);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);

    std::stringstream logMsg1;
    logMsg1 << "Detected \"" << filePath1 << "\" is infected with threatName (Open)";
    std::stringstream logMsg2;
    logMsg2 << "Detected \"" << filePath2 << "\" is infected with threatName (Close-Write)";
    EXPECT_TRUE(waitForLog(logMsg1.str()));
    EXPECT_TRUE(waitForLog(logMsg2.str()));
    ASSERT_EQ(socket->m_paths.size(), 2);
    scanRequestQueue->stop();
}


TEST_F(TestScanRequestHandler, scan_threadCanExitWhileWaiting)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scanRequestQueue = std::make_shared<ScanRequestQueue>();
    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket, m_mockFanotifyHandler);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);
    EXPECT_TRUE(waitForLog("Entering Main Loop"));
    scanRequestQueue->stop();
    scanHandlerThread->requestStopIfNotStopped();
}

TEST_F(TestScanRequestHandler, cleanScanOpen)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto socket = std::make_shared<RecordingMockSocket>(false, false);
    auto scanHandler = buildDefaultHandler(socket);

    EXPECT_CALL(*m_mockFanotifyHandler, cacheFd(_,_)).WillOnce(Return(0));

    scan_messages::ClientScanRequestPtr request(buildRequest());
    scanHandler->scan(request);

    EXPECT_EQ(socket->m_paths.size(), 1);
}
