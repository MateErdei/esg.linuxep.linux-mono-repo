// Copyright 2022 Sophos Limited. All rights reserved.

#include "sophos_on_access_process/onaccessimpl/OnAccessScanRequest.h"

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
