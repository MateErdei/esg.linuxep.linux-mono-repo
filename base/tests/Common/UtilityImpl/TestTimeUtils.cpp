// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "Common/UtilityImpl/RegexUtilities.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <future>
#include <regex>

using namespace Common::UtilityImpl;

TEST(TimeUtils, getCurrentTimeShouldReturnValidTime)
{
    EXPECT_GT(TimeUtils::getCurrTime(), 1556712000); // the compared time was 2019-05-01 12:00:00 UTC
}

TEST(TimeUtils, fromTimeShouldReturnSensibleResult)
{
    std::time_t currentTime = TimeUtils::getCurrTime();
    std::tm currentTimeTm;
    ::localtime_r(&currentTime, &currentTimeTm);
    std::string reportedString = TimeUtils::fromTime(currentTime);

    std::string pattern{ R"(\d{4}\d{2}\d{2} (\d{2})\d{2}\d{2})" };
    std::string matched = Common::UtilityImpl::returnFirstMatch(pattern, reportedString);
    EXPECT_FALSE(matched.empty()) << "reportedString: " << reportedString;

    std::string year{ std::to_string(currentTimeTm.tm_year + 1900) };
    std::string month{ std::to_string(currentTimeTm.tm_mon + 1) };
    std::string hour{ std::to_string(currentTimeTm.tm_hour) };
    std::string sec{ std::to_string(currentTimeTm.tm_sec) };
    EXPECT_GE(currentTimeTm.tm_year, 2019 - 1900);
    EXPECT_THAT(reportedString, ::testing::HasSubstr(year));
    EXPECT_THAT(reportedString, ::testing::HasSubstr(month));
    EXPECT_THAT(reportedString, ::testing::HasSubstr(hour));
    EXPECT_THAT(reportedString, ::testing::HasSubstr(sec));
}

TEST(TimeUtils, bootTimeShouldBeBeforeNow)
{
    std::time_t currentTime = TimeUtils::getCurrTime();
    std::time_t bootTime = TimeUtils::getBootTimeAsTimet();
    EXPECT_LT(bootTime, currentTime);
}

TEST(TimeUtils, toTimeInvertible)
{
    std::time_t currentTime = TimeUtils::getCurrTime();
    std::tm currentTimeTm{};
    ::localtime_r(&currentTime, &currentTimeTm);
    std::string reportedString = TimeUtils::fromTime(currentTime);

    std::time_t actualTime = TimeUtils::toTime(reportedString);

    EXPECT_EQ(currentTime, actualTime);
}

TEST(TimeUtils, MessageTimeStampShouldReturnSensibleResult)
{
    long epoch_time = 1600000000; //2020 09 13 12:26:40
    std::chrono::system_clock::time_point tp((std::chrono::seconds(epoch_time)));
    std::string convertedTime = Common::UtilityImpl::TimeUtils::MessageTimeStamp(tp);
    EXPECT_EQ(convertedTime, "2020-09-13T12:26:40.000Z");
}

TEST(TimeUtils, toEpochTimeShouldReturnSensibleResult)
{
    std::string convertedTime = Common::UtilityImpl::TimeUtils::toEpochTime("20200913 122640");

    EXPECT_EQ(convertedTime, "1600000000");
}

TEST(TimeUtils, toWindowsFileTimeShouldReturnSensibleResult)
{
    long epoch_time = 1600000000; //2020 09 13 12:26:40
    std::int64_t convertedTime = Common::UtilityImpl::TimeUtils::EpochToWindowsFileTime(epoch_time);
    EXPECT_EQ(convertedTime, 132444736000000000);
}

TEST(TimeUtils, WindowsFileTimeToEpochShouldReturnSensibleResult)
{
    int64_t file_time = 132444736000000000; //2020 09 13 12:26:40
    std::time_t convertedTime = Common::UtilityImpl::TimeUtils::WindowsFileTimeToEpoch(file_time);
    EXPECT_EQ(convertedTime, 1600000000);
}

