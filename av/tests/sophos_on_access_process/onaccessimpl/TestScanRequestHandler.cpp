// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessImplMemoryAppenderUsingTests.h"

#include "common/ThreadRunner.h"
#include "sophos_on_access_process/onaccessimpl/ScanRequestHandler.h"
#include "common/RecordingMockSocket.h"

#include <gtest/gtest.h>

#include <sstream>

using namespace ::testing;
using namespace sophos_on_access_process::onaccessimpl;

namespace
{
    class TestScanRequestHandler : public OnAccessImplMemoryAppenderUsingTests
    {
    };
}

TEST_F(TestScanRequestHandler, scanItem)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath = "/tmp/test";
    auto scanRequest = std::make_shared<ClientScanRequest>();
    scanRequest->setPath(filePath);
    datatypes::AutoFd fd;

    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        nullptr, socket);
    scanHandler->scan(scanRequest, fd);

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), filePath);
    std::stringstream logMsg;
    logMsg << "Detected " << filePath << " is infected with threatName";
    EXPECT_TRUE(appenderContains(""));
}

TEST_F(TestScanRequestHandler, scanThread)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const char* filePath = "/tmp/test";
    auto scanRequestQueue = std::make_shared<ScanRequestQueue>();
    auto scanRequest = std::make_shared<ClientScanRequest>();
    scanRequest->setPath(filePath);
    auto fd = std::make_shared<datatypes::AutoFd>();
    scanRequestQueue->emplace(std::make_pair(scanRequest, fd));

    auto socket = std::make_shared<RecordingMockSocket>();
    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
        scanRequestQueue, socket);
    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);

    std::stringstream logMsg;
    logMsg << "Detected \"" << filePath << "\" is infected with threatName";
    EXPECT_TRUE(waitForLog(logMsg.str()));
    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), filePath);
}