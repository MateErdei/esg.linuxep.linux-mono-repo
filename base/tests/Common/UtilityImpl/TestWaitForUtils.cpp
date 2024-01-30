// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include "Common/UtilityImpl/WaitForUtils.h"
#include <gtest/gtest.h>

using namespace Common::UtilityImpl;


TEST(WaitForUtils, waitLoopDoesNotThrowIfItReachesTheEndOftheLoop)
{
    auto simpleFunc = []() {return false;};
    double waitTime = 0.1;
    EXPECT_FALSE(Common::UtilityImpl::waitFor(waitTime,0.01,simpleFunc));
}

TEST(WaitForUtils, checkWaitLoopReturnsWhenConditionFulfilled)
{
    int counter = 0;
    auto simpleFunc = [&]()
    {
      counter++;
      return true;
    };
    double waitTime = 0.1;
    EXPECT_TRUE(Common::UtilityImpl::waitFor(waitTime,0.1,simpleFunc));

    ASSERT_EQ(counter, 1);
}

TEST(WaitForUtils, waitLoopExitsIfConditionFulfilled)
{
    int counter = 0;
    auto simpleFunc = [&]()
    {
      counter++;
      if (counter == 4)
      {
          return true;
      }
      return false;
    };
    double waitTime = 0.1;
    EXPECT_TRUE(Common::UtilityImpl::waitFor(waitTime,0.01,simpleFunc));
    ASSERT_EQ(counter, 4);
}
TEST(WaitForUtils, waitLoopRespectsTheIntervalAndWaitTimeGiven)
{
    int counter = 0;
    auto simpleFunc = [&]()
    {
        counter++;
        return false;
    };
    double waitTime = 0.1;
    EXPECT_FALSE(Common::UtilityImpl::waitFor(waitTime,0.01,simpleFunc));
    ASSERT_GE(counter, 10);
}
