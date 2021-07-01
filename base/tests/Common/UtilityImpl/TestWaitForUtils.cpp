/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/UtilityImpl/WaitForUtils.h>
#include <gtest/gtest.h>

using namespace Common::UtilityImpl;


TEST(WaitForUtils, waitLoopDoesNotThrowIfItReachesTheEndOftheLoop) // NOLINT
{
    auto simpleFunc = []() {return false;};
    double waitTime = 0.1;
    EXPECT_NO_THROW(Common::UtilityImpl::waitFor(waitTime,0.01,simpleFunc));
}

TEST(WaitForUtils, checkWaitLoopReturnsWhenConditionFulfiled) // NOLINT
{
    int counter = 0;
    auto simpleFunc = [&]()
    {
      counter++;
      return true;
    };
    double waitTime = 0.1;
    EXPECT_NO_THROW(Common::UtilityImpl::waitFor(waitTime,0.1,simpleFunc));

    ASSERT_EQ(counter, 1);
}

TEST(WaitForUtils, waitLoopRespectsTheIntervalAndWaitTimeGiven) // NOLINT
{
    int counter = 0;
    auto simpleFunc = [&]()
    {
        counter++;
        return false;
    };
    double waitTime = 0.1;
    EXPECT_NO_THROW(Common::UtilityImpl::waitFor(waitTime,0.01,simpleFunc));
    ASSERT_GE(counter, 10);
}
