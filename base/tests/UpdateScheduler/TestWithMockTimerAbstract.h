/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <Common/UtilityImpl/TimeUtils.h>
#include <UpdateScheduler/ScheduledUpdate.h>
#include <gmock/gmock-matchers.h>
#include <tests/Common/Helpers/FakeTimeUtils.h>

class TestWithMockTimerAbstract : public ::testing::Test
{
public:
    TestWithMockTimerAbstract() : t_20190501T13h(calculateRequiredTestDateTime()) {}
    std::time_t t_20190501T13h; //{1556712000}; // Wednesday at 13:00
    std::time_t minute{ 60 };
    std::time_t hour{ 60 * minute };
    std::time_t week{ hour * 24 * 7 };
    std::string reportedScheduledTime{ "20190501 130000" };
    std::string reportedNextScheduledTime{ "20190508 130000" };

    Common::UtilityImpl::ScopedReplaceITime setupSimulatedTimes(
        std::vector<time_t> simulated_times,
        std::function<void()> function = []() {})
    {
        std::unique_ptr<Common::UtilityImpl::ITime> mockTimer{ new SequenceOfFakeTime(
            simulated_times, std::chrono::milliseconds(5), function) };
        return Common::UtilityImpl::ScopedReplaceITime(std::move(mockTimer));
    }

    UpdateScheduler::ScheduledUpdate::WeekDayAndTimeForDelay getScheduledTime()
    {
        return { .weekDay = 3, .hour = 13, .minute = 0 };
    }

    std::time_t calculateRequiredTestDateTime()
    {
        // Wednesday at 2019-05-01 13:00
        std::tm testDateTime{};
        testDateTime.tm_year = 2019 - 1900;
        testDateTime.tm_mon = 4;
        testDateTime.tm_mday = 1;
        testDateTime.tm_hour = 13;
        testDateTime.tm_isdst = -1;

        return mktime(&testDateTime);
    }
};
