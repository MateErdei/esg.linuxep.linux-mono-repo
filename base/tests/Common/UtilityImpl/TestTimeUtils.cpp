/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/UtilityImpl/RegexUtilities.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <regex>
#include <include/gmock/gmock-matchers.h>
#include <future>

using namespace Common::UtilityImpl;

TEST(TimeUtils, getCurrentTimeShouldReturnValidTime) // NOLINT
{
    EXPECT_GT(TimeUtils::getCurrTime(), 1556712000); // the compared time was 2019-05-01 12:00:00 UTC
}


TEST(TimeUtils, fromTimeShouldReturnSensibleResult) // NOLINT
{
    std::time_t currentTime = TimeUtils::getCurrTime();
    std::tm currentTimeTm;
    ::localtime_r(&currentTime, &currentTimeTm);
    std::string reportedString = TimeUtils::fromTime(currentTime);

    std::string pattern{ R"(\d{4}\d{2}\d{2} (\d{2})\d{2}\d{2})"};
    std::string matched = Common::UtilityImpl::returnFirstMatch(pattern, reportedString );
    EXPECT_FALSE(matched.empty()) << "reportedString: " << reportedString;

    std::string year{ std::to_string(currentTimeTm.tm_year + 1900) };
    std::string month{ std::to_string(currentTimeTm.tm_mon+1) };
    std::string hour{ std::to_string(currentTimeTm.tm_hour) };
    std::string sec{ std::to_string(currentTimeTm.tm_sec) };
    EXPECT_GE( currentTimeTm.tm_year, 2019-1900);
    EXPECT_THAT(reportedString, ::testing::HasSubstr(year));
    EXPECT_THAT(reportedString, ::testing::HasSubstr(month));
    EXPECT_THAT(reportedString, ::testing::HasSubstr(hour));
    EXPECT_THAT(reportedString, ::testing::HasSubstr(sec));
}

TEST(TimeUtils, bootTimeShouldBeBeforeNow) // NOLINT
{
    std::time_t currentTime = TimeUtils::getCurrTime();
    std::time_t bootTime = TimeUtils::getBootTimeAsTimet();
    EXPECT_LT(bootTime, currentTime);
}


