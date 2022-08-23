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
    datatypes::AutoFd fd1(1);
    datatypes::AutoFd fd2(2);

    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.push(scanRequest1, fd1));
    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.push(scanRequest2, fd2));
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
    datatypes::AutoFd fd1(1);
    datatypes::AutoFd fd2(2);

    queue.push(pushRequest1, fd1);
    queue.push(pushRequest2, fd2);
    auto popItem1 = queue.pop();
    EXPECT_EQ(popItem1.first->getPath(), "1");
    EXPECT_EQ(popItem1.second.fd(), 1);
    auto popItem2 = queue.pop();
    EXPECT_EQ(popItem2.first->getPath(), "2");
    EXPECT_EQ(popItem2.second.fd(), 2);
}

TEST_F(TestScanRequestQueue, queueCanProcessMoreItemsThanItsMaxSizeSequentially)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanRequestQueue queue(2);
    auto pushRequest1 = std::make_shared<ClientScanRequest>();
    pushRequest1->setPath("1");
    auto pushRequest2 = std::make_shared<ClientScanRequest>();
    pushRequest2->setPath("2");
    auto pushRequest3 = std::make_shared<ClientScanRequest>();
    pushRequest3->setPath("3");
    datatypes::AutoFd fd1(1);
    datatypes::AutoFd fd2(2);
    datatypes::AutoFd fd3(3);

    EXPECT_TRUE(queue.push(pushRequest1, fd1));
    EXPECT_TRUE(queue.push(pushRequest2, fd2));
    EXPECT_FALSE(queue.push(pushRequest3, fd3));
    auto popItem1 = queue.pop();
    EXPECT_EQ(popItem1.first->getPath(), "1");
    EXPECT_EQ(popItem1.second.fd(), 1);
    EXPECT_TRUE(queue.push(pushRequest3, fd3));
    auto popItem2 = queue.pop();
    EXPECT_EQ(popItem2.first->getPath(), "2");
    EXPECT_EQ(popItem2.second.fd(), 2);
    auto popItem3 = queue.pop();
    EXPECT_EQ(popItem3.first->getPath(), "3");
    EXPECT_EQ(popItem3.second.fd(), 3);
}
