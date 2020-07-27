/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "datatypes/Time.h"

using namespace datatypes;

TEST(TestTime, TestCurrentToCentralTime) // NOLINT
{
    Time::currentToCentralTime();
}

TEST(TestTime, TestEpochToCentralTimeThrowsFailedToConvert) // NOLINT
{
    try
    {
        Time::epochToCentralTime(-111111111111111111);
    }
    catch (std::runtime_error& e)
    {
        EXPECT_EQ(e.what(),std::string("Failed to convert timestamp to UTC"));
    }
}

TEST(TestTime, TestEpochToCentralTimeThrowsFailedToFormat) // NOLINT
{
    try
    {
        Time::epochToCentralTime(9999999999999999);
    }
    catch (std::runtime_error& e)
    {
        EXPECT_EQ(e.what(),std::string("Failed to format timestamp"));
    }
}