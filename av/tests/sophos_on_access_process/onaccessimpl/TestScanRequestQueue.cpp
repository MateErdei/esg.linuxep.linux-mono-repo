// Copyright 2022, Sophos Limited.  All rights reserved.

#define TEST_PUBLIC public

// Product
#include "sophos_on_access_process/onaccessimpl/ScanRequestQueue.h"
#include "scan_messages/ClientScanRequest.h"
// Test
#include "OnAccessImplMemoryAppenderUsingTests.h"
#include "datatypes/MockSysCalls.h"

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
}

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

TEST_F(TestScanRequestQueue, dedupRemovesDuplicate)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    struct ::stat stat1{};
    stat1.st_ino = 1;
    stat1.st_dev = 0;
    EXPECT_CALL(*sysCallWrapper, fstat(10, _)).WillOnce(DoAll(SetArgPointee<1>(stat1), Return(0)));

    struct ::stat stat2{};
    stat2.st_ino = 1;
    stat2.st_dev = 0;
    EXPECT_CALL(*sysCallWrapper, fstat(20, _)).WillOnce(DoAll(SetArgPointee<1>(stat2), Return(0)));

    ScanRequestQueue queue(3);

    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ClientScanRequest>(sysCallWrapper, fd);
    emplaceRequest1->setPath("1");

    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ClientScanRequest>(sysCallWrapper, fd);
    emplaceRequest2->setPath("2");

    queue.emplace(emplaceRequest1);
    ASSERT_EQ(queue.m_deDupData.size(), 1); // Something on the dedup map

    queue.emplace(emplaceRequest2);
    // Second request de-duped
    EXPECT_EQ(queue.size(), 1);
}

TEST_F(TestScanRequestQueue, dedupCanBeTurnedOff)
{
    // Nothing should be calling into this if dedup is turned off
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    ScanRequestQueue queue(3, false);

    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ClientScanRequest>(sysCallWrapper, fd);
    emplaceRequest1->setPath("1");

    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ClientScanRequest>(sysCallWrapper, fd);
    emplaceRequest2->setPath("2");

    queue.emplace(emplaceRequest1);
    ASSERT_EQ(queue.m_deDupData.size(), 0);

    queue.emplace(emplaceRequest2);
    // Second request not de-duped
    EXPECT_EQ(queue.size(), 2);
}

TEST_F(TestScanRequestQueue, dedupWillHandleHashCollision)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    struct ::stat stat1{};
    stat1.st_ino = 1;
    stat1.st_dev = 0;
    EXPECT_CALL(*sysCallWrapper, fstat(10, _)).WillOnce(DoAll(SetArgPointee<1>(stat1), Return(0)));

    struct ::stat stat2{};
    stat2.st_ino = 2;
    stat2.st_dev = 0;
    EXPECT_CALL(*sysCallWrapper, fstat(20, _)).WillOnce(DoAll(SetArgPointee<1>(stat2), Return(0)));

    ScanRequestQueue queue(3);

    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ClientScanRequest>(sysCallWrapper, fd);
    emplaceRequest1->setPath("1");

    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ClientScanRequest>(sysCallWrapper, fd);
    emplaceRequest2->setPath("2");

    auto hash = emplaceRequest2->hash();
    ASSERT_TRUE(hash.has_value());
    queue.m_deDupData[hash.value()] = emplaceRequest1->uniqueMarker();

    queue.emplace(emplaceRequest2);
    // Even though the deDupData matches on hash value, we still don't de-dup due to the unique marker
    EXPECT_EQ(queue.size(), 1);
}
