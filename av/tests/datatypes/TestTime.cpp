/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "datatypes/Time.h"
using namespace testing;
using namespace datatypes;

TEST(TestTime, CurrentToCentralTime) // NOLINT
{
    ASSERT_NE(Time::currentToCentralTime(), "");
}

TEST(TestTime, EpochToCentralTimeThrowsFailedToConvert) // NOLINT
{
    try
    {
        Time::epochToCentralTime(-111111111111111111);
        FAIL() << "Did not throw exception as expected";
    }
    catch (std::runtime_error& e)
    {
        EXPECT_EQ(e.what(),std::string("Failed to convert timestamp to UTC"));
    }
    catch (...) {
        FAIL() <<  "Unexpected exception type";
    }
}

TEST(TestTime, EpochToCentralTimeThrowsFailedToFormat) // NOLINT
{
    try
    {
        Time::epochToCentralTime(9999999999999999);
        FAIL() << "Did not throw exception as expected";
    }
    catch (std::runtime_error& e)
    {
        EXPECT_EQ(e.what(),std::string("Failed to format timestamp"));
    }
    catch (...) {
        FAIL() <<  "Unexpected exception type";
    }
}