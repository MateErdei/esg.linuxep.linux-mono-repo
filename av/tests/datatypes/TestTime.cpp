/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "datatypes/Time.h"

using namespace datatypes;

TEST(TestTime, CurrentToCentralTime) // NOLINT
{
    Time::currentToCentralTime();
}

TEST(TestTime, EpochToCentralTimeThrowsFailedToConvert) // NOLINT
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

TEST(TestTime, EpochToCentralTimeThrowsFailedToFormat) // NOLINT
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