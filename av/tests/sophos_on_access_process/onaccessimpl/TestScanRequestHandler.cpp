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

//TEST_F(TestScanRequestHandler, scanThread)
//{
//    auto scanRequestQueue = std::make_shared<ScanRequestQueue>(1);
//    auto pushRequest1 = std::make_shared<ClientScanRequest>();
//    pushRequest1->setPath("/tmp/test");
//    scanRequestQueue->push(pushRequest1, 1);
//
//    auto socket = std::make_shared<RecordingMockSocket>();
//    auto scanHandler = std::make_shared<sophos_on_access_process::onaccessimpl::ScanRequestHandler>(
//        scanRequestQueue, socket);
//    auto scanHandlerThread = std::make_shared<common::ThreadRunner>(scanHandler, "scanHandler", true);
//
//    ASSERT_EQ(socket->m_paths.size(), 1);
//    EXPECT_EQ(socket->m_paths.at(0), "/tmp/test");
//}