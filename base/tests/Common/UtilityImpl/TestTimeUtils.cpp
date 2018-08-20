/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/TimeUtils.h>
#include <chrono>
#include <thread>

#include "gtest/gtest.h"

using namespace Common::UtilityImpl;

int advanceFromLocalToUTC()
{
    time_t current = time(nullptr);
    struct tm localTime;
    struct tm utcTime;
    localtime_r(&current, &localTime );
    gmtime_r(&current, &utcTime);
    utcTime.tm_isdst = 0;

    time_t recoverLocal = mktime(&localTime);
    time_t recoverUTC = mktime(&utcTime);
    if( utcTime.tm_isdst >0 )
    {
        recoverUTC += 3600;
    }
    return recoverUTC - recoverLocal;
}


time_t extractTimeFromString( const std::string & stringRepresentedTime)
{
    assert(stringRepresentedTime.size() == 15);
    struct tm localtimeTM;
    strptime(stringRepresentedTime.data(), "%Y%m%d %H%M%S", &localtimeTM);

    time_t localTimeT = mktime(&localtimeTM);
    time_t utcTime = localTimeT - advanceFromLocalToUTC();
    return utcTime;

}


TEST(TestTimeUtils, extractTimeFromString) // NOLINT
{
    std::string timestamp = "20180816 153800"; // local time in uk
    time_t time1 = 1534430280;
    EXPECT_EQ( extractTimeFromString(timestamp),  time1);
    EXPECT_EQ( TimeUtils::fromTime(time1), timestamp);
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

