// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessImplMemoryAppenderUsingTests.h"

#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"
#include "scan_messages/ClientScanRequest.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace sophos_on_access_process::onaccessimpl;

namespace
{
    class TestScanRequestQueue : public OnAccessImplMemoryAppenderUsingTests
    {
    };
}

TEST_F(TestScanRequestQueue, push_onlyEnqueuesUpToMaxSize)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanRequestQueue queue(1);
    auto scanRequest1 = std::make_shared<ClientScanRequest>();
    scanRequest1->setUserID("1");
    auto scanRequest2 = std::make_shared<ClientScanRequest>();
    scanRequest2->setUserID("2");

    EXPECT_EQ(queue.size(), 0);
    queue.push(scanRequest1);
    EXPECT_EQ(queue.size(), 1);
    queue.push(scanRequest2);
    EXPECT_EQ(queue.size(), 1);

    EXPECT_TRUE(waitForLog("Unable to add scan request to queue as it is"));
}

TEST_F(TestScanRequestQueue, push_FIFO)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanRequestQueue queue(2);
    auto pushRequest1 = std::make_shared<ClientScanRequest>();
    pushRequest1->setPath("1");
    auto pushRequest2 = std::make_shared<ClientScanRequest>();
    pushRequest2->setPath("2");

    queue.push(pushRequest1);
    queue.push(pushRequest2);
    auto popRequest1 = queue.pop();
    EXPECT_EQ(popRequest1->getPath(), "1");
    auto popRequest2 = queue.pop();
    EXPECT_EQ(popRequest2->getPath(), "2");
}
