/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateScheduler/ScheduledUpdate.h>
#include <gmock/gmock-matchers.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <tests/Common/Helpers/FakeTimeUtils.h>
#include <thread>

using namespace UpdateScheduler;
using milliseconds = std::chrono::milliseconds;
using minutes = std::chrono::minutes;
using time_point = std::chrono::steady_clock::time_point;

namespace
{
    Common::UtilityImpl::ScopedReplaceITime setupSimulatedTimes( std::vector<time_t> simulated_times )
    {
        std::unique_ptr<Common::UtilityImpl::ITime> mockTimer{
                new SequenceOfFakeTime( simulated_times, std::chrono::milliseconds(5), [](){ })
        };
        return Common::UtilityImpl::ScopedReplaceITime( std::move(mockTimer));
    }

    std::time_t t_20190501T13h{1556712000}; // Wednesday at 13:00
    std::time_t minute{60};
    std::time_t hour{ 60 * minute };
    std::time_t week{ hour * 24 * 7};
    std::string reportedScheduledTime{"20190501 130000"};
    std::string reportedNextScheduledTime{"20190508 130000"};
    std::tm getScheduledTime()
    {
        std::tm tm_1{};
        tm_1.tm_wday=3;
        tm_1.tm_hour=13;
        return tm_1;
    }
}

TEST(ScheduledUpdate, NextUpdateTimeShouldAlwaysBeAfterCurrentTime) // NOLINT
{
    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);
    std::vector<std::pair<std::time_t, std::string>> simulated_current_time_and_expected_nextScheduledTime{
            {t_20190501T13h-1, reportedScheduledTime},
            {t_20190501T13h-5*hour, reportedScheduledTime},
            {t_20190501T13h-week+1, reportedScheduledTime},
            {t_20190501T13h+1, reportedNextScheduledTime},
            {t_20190501T13h+week-1, reportedNextScheduledTime},
    };
    for( auto & time_expected : simulated_current_time_and_expected_nextScheduledTime)
    {
        std::vector<std::time_t> simulated_time{ time_expected.first };
        std::string expectedNextTimeReported{time_expected.second};

        auto scopedReplaceTime{ setupSimulatedTimes(simulated_time) };

        scheduledUpdate.setScheduledTime(getScheduledTime());
        EXPECT_EQ( scheduledUpdate.nextUpdateTime(), expectedNextTimeReported );
    }

}



TEST(ScheduledUpdate, TimeToUpdate) // NOLINT
{
    struct TimeToUpdateExpectance
    {
        std::time_t simulatedTime;
        int offsetInMinutes;
        bool expectedAnswer;
    };


    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);
    std::vector<TimeToUpdateExpectance> check_time_to_update{
            {.simulatedTime=t_20190501T13h+1, .offsetInMinutes=0, .expectedAnswer=true},
            {.simulatedTime=t_20190501T13h-1, .offsetInMinutes=0, .expectedAnswer=false}, // not time yet
            {.simulatedTime=t_20190501T13h+1, .offsetInMinutes=1, .expectedAnswer=false}, // it needs one 59 seconds more
            {.simulatedTime=t_20190501T13h+hour, .offsetInMinutes=1, .expectedAnswer=true} // some time has already passed
    };
    // pretend scheduledUpdate was created one hour before
    {
        auto scopedReplaceTime{ setupSimulatedTimes({t_20190501T13h - hour}) };
        scheduledUpdate.setScheduledTime(getScheduledTime());
    }

    for( auto & expected : check_time_to_update)
    {
        std::vector<std::time_t> simulated_time{  expected.simulatedTime };
        auto scopedReplaceTime{ setupSimulatedTimes(simulated_time) };
        EXPECT_EQ( scheduledUpdate.timeToUpdate(expected.offsetInMinutes ), expected.expectedAnswer);
    }

}

TEST(ScheduledUpdate, missedUpdate) // NOLINT
{
    struct MissedUpdateExpectance
    {
        std::time_t simulatedTime;
        std::string reportedLastUpdate;
        bool expectedAnswer;
    };


    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);
    std::vector<MissedUpdateExpectance> check_time_to_update{
            {.simulatedTime=t_20190501T13h+1, .reportedLastUpdate="20190501 12:00:00", .expectedAnswer=false},
            {.simulatedTime=t_20190501T13h+1, .reportedLastUpdate="20190429 12:00:00", .expectedAnswer=false},
            {.simulatedTime=t_20190501T13h+1, .reportedLastUpdate="20190424 12:00:00", .expectedAnswer=false}, // it gives an hour as the update will happen within the next hour anyway.
            {.simulatedTime=t_20190501T13h+1, .reportedLastUpdate="20190424 11:59:59", .expectedAnswer=true},
            {.simulatedTime=t_20190501T13h+1, .reportedLastUpdate="20190101 11:59:59", .expectedAnswer=true},
    };

    // pretend scheduledUpdate was created one hour before
    {
        auto scopedReplaceTime{ setupSimulatedTimes({t_20190501T13h - hour}) };
        scheduledUpdate.setScheduledTime(getScheduledTime());
    }

    for( auto & expected : check_time_to_update)
    {
        std::vector<std::time_t> simulated_time{  expected.simulatedTime };
        auto scopedReplaceTime{ setupSimulatedTimes(simulated_time) };
        EXPECT_EQ( scheduledUpdate.missedUpdate(expected.reportedLastUpdate ), expected.expectedAnswer);
    }

}


TEST(ScheduledUpdate, confirmUpdatedTime) // NOLINT
{

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    // pretend scheduledUpdate was created one hour before
    {
        auto scopedReplaceTime{ setupSimulatedTimes({t_20190501T13h - hour}) };
        scheduledUpdate.setScheduledTime(getScheduledTime());
    }

    std::vector<std::time_t> simulated_time;
    for( int i=-1; i<40; i++)
    {
        simulated_time.push_back(t_20190501T13h + i*minute);
    }

    auto scopedReplaceTime{ setupSimulatedTimes(simulated_time) };

    int countTimeToUpdate = 0;
    for( int i=0; i<50; i++)
    {
        if( scheduledUpdate.timeToUpdate(15))
        {
            countTimeToUpdate++;
            scheduledUpdate.confirmUpdatedTime(); // confirm updatedtime is to be called after timeToUpdate returns true
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));// to enable simulatedTime to move to the next time
    }
    EXPECT_EQ(countTimeToUpdate, 1);
    EXPECT_EQ(scheduledUpdate.nextUpdateTime(), reportedNextScheduledTime);

}



