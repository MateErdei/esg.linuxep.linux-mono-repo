// Copyright 2022 Sophos Limited. All rights reserved.

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

using ScanRequest_t = ScanRequestQueue::scan_request_t;
using ScanRequestPtr = ScanRequestQueue::scan_request_ptr_t;

namespace
{
    class TestScanRequestQueue : public OnAccessImplMemoryAppenderUsingTests
    {
    protected:
        static ScanRequestPtr emptyRequest()
        {
            return std::make_shared<ScanRequest_t>();
        }
    };
}

TEST_F(TestScanRequestQueue, push_onlyEnqueuesUpToMaxSize)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ScanRequestQueue queue(1);
    auto emplaceRequest1 = emptyRequest();
    emplaceRequest1->setUserID("1");
    auto emplaceRequest2 = emptyRequest();
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
    auto emplaceRequest1 = emptyRequest();
    emplaceRequest1->setPath("1");
    auto emplaceRequest2 = emptyRequest();
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
    auto emplaceRequest1 = emptyRequest();
    emplaceRequest1->setPath("1");
    auto emplaceRequest2 = emptyRequest();
    emplaceRequest2->setPath("2");
    auto emplaceRequest3 = emptyRequest();
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

    EXPECT_CALL(*sysCallWrapper, fstat(10, _)).WillOnce(fstatReturnsDeviceAndInode(0, 1));
    EXPECT_CALL(*sysCallWrapper, fstat(20, _)).WillOnce(fstatReturnsDeviceAndInode(0, 1));

    ScanRequestQueue queue(3);

    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    emplaceRequest1->setPath("1");

    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    emplaceRequest2->setPath("2");

    queue.emplace(emplaceRequest1);
    ASSERT_EQ(queue.m_deDupData.size(), 1); // Something on the dedup map
    EXPECT_EQ(queue.size(), 1);

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
    auto emplaceRequest1 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    emplaceRequest1->setPath("1");

    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
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

    EXPECT_CALL(*sysCallWrapper, fstat(10, _)).WillOnce(fstatReturnsDeviceAndInode(0, 1));
    EXPECT_CALL(*sysCallWrapper, fstat(20, _)).WillOnce(fstatReturnsDeviceAndInode(0, 2));

    ScanRequestQueue queue(3);

    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    emplaceRequest1->setPath("1");

    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    emplaceRequest2->setPath("2");

    auto hash = emplaceRequest2->hash();
    ASSERT_TRUE(hash.has_value());
    queue.m_deDupData[hash.value()] = emplaceRequest1->uniqueMarker();

    queue.emplace(emplaceRequest2);
    // Even though the deDupData matches on hash value, we still don't de-dup due to the unique marker
    EXPECT_EQ(queue.size(), 1);
}

TEST_F(TestScanRequestQueue, highBitDifferentDeviceAreNotSkipped)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    EXPECT_CALL(*sysCallWrapper, fstat(10, _))
        .WillOnce(fstatReturnsDeviceAndInode(0x0000000000000001,0x0000000000000001));

    EXPECT_CALL(*sysCallWrapper, fstat(20, _))
        .WillOnce(fstatReturnsDeviceAndInode(0x8000000000000001,0x0000000000000001));

    ScanRequestQueue queue(3);

    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);

    queue.emplace(emplaceRequest1);
    queue.emplace(emplaceRequest2);
    // Second request not de-duped
    EXPECT_EQ(queue.size(), 2);
}

TEST_F(TestScanRequestQueue, returnsTrueIfSizeIsBelowMaxSizeMinusLogNotFullThreshold)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    ScanRequestQueue queue(10, false);
    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);

    queue.emplace(emplaceRequest1);
    queue.emplace(emplaceRequest2);

    ASSERT_EQ(queue.size(), 2);

    EXPECT_TRUE(queue.sizeIsLessThan(5));
}


TEST_F(TestScanRequestQueue, returnsTrueIfSizeIsSameAsMaxSizeMinusLogNotFullThreshold)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    ScanRequestQueue queue(7, false);
    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);

    queue.emplace(emplaceRequest1);
    queue.emplace(emplaceRequest2);

    ASSERT_EQ(queue.size(), 2);

    EXPECT_TRUE(queue.sizeIsLessThan(5));
}

TEST_F(TestScanRequestQueue, returnsFalseIfSizeIsGreaterThanMaxSizeMinusLogNotFullThreshold)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    ScanRequestQueue queue(7, false);
    datatypes::AutoFd fd{10};
    auto emplaceRequest1 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    fd.reset(20);
    auto emplaceRequest2 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    fd.reset(30);
    auto emplaceRequest3 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);
    fd.reset(40);
    auto emplaceRequest4 = std::make_shared<ScanRequest_t>(sysCallWrapper, fd);

    queue.emplace(emplaceRequest1);
    queue.emplace(emplaceRequest2);
    queue.emplace(emplaceRequest3);
    queue.emplace(emplaceRequest4);

    ASSERT_EQ(queue.size(), 4);

    EXPECT_FALSE(queue.sizeIsLessThan(5));
}