// Copyright 2022 Sophos Limited. All rights reserved.

#include "sophos_on_access_process/onaccessimpl/OnAccessScanRequest.h"

#include "datatypes/MockSysCalls.h"

#include <gtest/gtest.h>

using sophos_on_access_process::onaccessimpl::OnAccessScanRequest;

TEST(TestOnAccessScanRequest, hasTime)
{
    OnAccessScanRequest scanRequest{};
    auto creationTime = scanRequest.getCreationTime().time_since_epoch().count();

    EXPECT_NE(creationTime, 0);
}

TEST(TestOnAccessScanRequest, setsQueueSize)
{
    OnAccessScanRequest scanRequest{};

    size_t testValue = 1439;

    scanRequest.setQueueSizeAtTimeOfInsert(testValue);

    EXPECT_EQ(testValue, scanRequest.getQueueSizeAtTimeOfInsert());
}

TEST(TestOnAccessScanRequest, noHashWithNoFd)
{
    OnAccessScanRequest clientScanRequest{};
    auto hash = clientScanRequest.hash();
    EXPECT_FALSE(hash.has_value());
}

TEST(TestOnAccessScanRequest, hashWithFstat)
{
    auto sysCallWrapper = std::make_shared<StrictMock<MockSystemCallWrapper>>();

    constexpr int FD = 9999999;

    struct ::stat stat1{};
    stat1.st_ino = 1;
    stat1.st_dev = 1;
    EXPECT_CALL(*sysCallWrapper, fstat(FD, _)).WillOnce(DoAll(SetArgPointee<1>(stat1), Return(0)));

    datatypes::AutoFd fd{FD};
    OnAccessScanRequest clientScanRequest{sysCallWrapper, fd};
    auto hash = clientScanRequest.hash();
    ASSERT_TRUE(hash.has_value());
    EXPECT_NE(hash.value(), 0);
}
