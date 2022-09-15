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
    auto emplaceRequest1 = std::make_shared<ClientScanRequest>();
    emplaceRequest1->setUserID("1");
    auto emplaceRequest2 = std::make_shared<ClientScanRequest>();
    emplaceRequest2->setUserID("2");

    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.emplace(emplaceRequest1));
    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.emplace(emplaceRequest2));
    EXPECT_EQ(queue.size(), 1);

    EXPECT_TRUE(waitForLog("Unable to add scan request to queue as it is"));
}

/*
TEST_F(TestScanRequestQueue, reportsWhenEmptied)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanRequestQueue queue(1);
    auto emplaceRequest1 = std::make_shared<ClientScanRequest>();
    emplaceRequest1->setUserID("1");

    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.emplace(emplaceRequest1));
    EXPECT_EQ(queue.size(), 1);
    EXPECT_NE(queue.pop(), nullptr);


    EXPECT_TRUE(waitForLog("Scan Queue is empty"));
}
*/

TEST_F(TestScanRequestQueue, push_FIFO)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanRequestQueue queue(2);
    auto emplaceRequest1 = std::make_shared<ClientScanRequest>();
    emplaceRequest1->setPath("1");
    auto emplaceRequest2 = std::make_shared<ClientScanRequest>();
    emplaceRequest2->setPath("2");

    queue.emplace(emplaceRequest1);
    queue.emplace(emplaceRequest2);
    auto popItem1 = queue.pop();
    ASSERT_TRUE(popItem1 != nullptr);
    EXPECT_EQ(popItem1->getPath(), "1");
    auto popItem2 = queue.pop();
    ASSERT_TRUE(popItem2 != nullptr);
    EXPECT_EQ(popItem2->getPath(), "2");
}

TEST_F(TestScanRequestQueue, queueCanProcessMoreItemsThanItsMaxSizeSequentially)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanRequestQueue queue(2);
    auto emplaceRequest1 = std::make_shared<ClientScanRequest>();
    emplaceRequest1->setPath("1");
    auto emplaceRequest2 = std::make_shared<ClientScanRequest>();
    emplaceRequest2->setPath("2");
    auto emplaceRequest3 = std::make_shared<ClientScanRequest>();
    emplaceRequest3->setPath("3");

    EXPECT_TRUE(queue.emplace(emplaceRequest1));
    EXPECT_TRUE(queue.emplace(emplaceRequest2));
    EXPECT_FALSE(queue.emplace(emplaceRequest3));
    auto popItem1 = queue.pop();
    ASSERT_TRUE(popItem1 != nullptr);
    EXPECT_EQ(popItem1->getPath(), "1");
    EXPECT_TRUE(queue.emplace(emplaceRequest3));
    auto popItem2 = queue.pop();
    ASSERT_TRUE(popItem2 != nullptr);
    EXPECT_EQ(popItem2->getPath(), "2");
    auto popItem3 = queue.pop();
    ASSERT_TRUE(popItem3 != nullptr);
    EXPECT_EQ(popItem3->getPath(), "3");
}
