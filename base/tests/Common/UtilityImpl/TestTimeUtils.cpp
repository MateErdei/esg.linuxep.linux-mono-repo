/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/TimeUtils.h>
#include <chrono>
#include <thread>

#include "gtest/gtest.h"

using namespace Common::UtilityImpl;

time_t extractTimeFromString( const std::string & stringRepresentedTime)
{
    assert(stringRepresentedTime.size() == 15);
    struct tm tm;
    strptime(stringRepresentedTime.data(), "%Y%m%d %H%M%S", &tm);
    time_t t = mktime(&tm);
    if( tm.tm_isdst)
    {
        t -= 3600;
    }
    return t;
}


TEST(TestTimeUtils, extractTimeFromString) // NOLINT
{
    std::string timestamp = "20180816 153800";
    time_t time1 = 1534430280;
    ASSERT_EQ( extractTimeFromString(timestamp),  time1);
    ASSERT_EQ( TimeUtils::fromTime(time1), timestamp);
}
/**
 * Disabled because we should not be running continuously these google tests.
 */
TEST(TestTimeUtils, DISABLED_getCurrTime) // NOLINT
{
    time_t t1 = TimeUtils::getCurrTime();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    time_t t2 = TimeUtils::getCurrTime();
    EXPECT_LE(t1 , t2);
    EXPECT_LE(t2 , t1+3);
}

/**
 * Disabled becasue we should not be running continuously these google tests. 
 */
TEST(TestTimeUtils, DISABLED_getBootTime) // NOLINT
{

    time_t t1 = TimeUtils::getCurrTime();
    time_t t2 = TimeUtils::getBootTimeAsTimet();
    // boot time was before now
    EXPECT_LE(t2 , t1);
    // boot time was after year ago
    EXPECT_LE(t1 - 3600*365 , t2);
}

