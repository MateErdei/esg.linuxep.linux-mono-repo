// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponseActionsImpl/ActionsUtils.h"
#include "modules/Common/UtilityImpl/TimeUtils.h"

#include "tests/Common/Helpers/MemoryAppender.h"


#include <gtest/gtest.h>
#include <gmock/gmock.h>

class ActionsUtilsTests : public MemoryAppenderUsingTests
{
public:
    ActionsUtilsTests()
        : MemoryAppenderUsingTests("responseactions")
    {}
};

TEST_F(ActionsUtilsTests, testExpiry)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    EXPECT_TRUE(actionsUtils.isExpired(1000));
    EXPECT_FALSE(actionsUtils.isExpired(1991495566));
    Common::UtilityImpl::FormattedTime time;
    u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();
    EXPECT_TRUE(actionsUtils.isExpired(currentTime-10));
    EXPECT_FALSE(actionsUtils.isExpired(currentTime+10));
}