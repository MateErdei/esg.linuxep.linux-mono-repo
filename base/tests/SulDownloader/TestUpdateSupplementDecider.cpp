/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigurationDataBase.h"

#include <SulDownloader/UpdateSupplementDecider.h>

using namespace SulDownloader;

namespace
{
    class TestUpdateSupplementDecider : public ConfigurationDataBase
    {
    };
}

TEST_F(TestUpdateSupplementDecider, constructor) // NOLINT
{
    UpdateSupplementDecider::WeekDayAndTimeForDelay delay;
    UpdateSupplementDecider foo(delay);
    static_cast<void>(foo);
}

TEST_F(TestUpdateSupplementDecider, disabledScheduleAlwaysUpdatesProducts) // NOLINT
{
    UpdateSupplementDecider::WeekDayAndTimeForDelay delay;
    UpdateSupplementDecider foo(delay);
    EXPECT_TRUE(foo.updateProducts());
    EXPECT_TRUE(foo.updateProducts());
}

namespace
{
    class PublicUpdateSupplementDecider : public UpdateSupplementDecider
    {
    public:
        using UpdateSupplementDecider::UpdateSupplementDecider;

        time_t getCurrentTime() override
        {
            return UpdateSupplementDecider::getCurrentTime();
        }
        time_t getLastSuccessfulProductUpdate() override
        {
            return UpdateSupplementDecider::getLastSuccessfulProductUpdate();
        }

        time_t public_lastScheduledProductUpdate()
        {
            return lastScheduledProductUpdate();
        }
    };

    class FixedLastSuccessfulProductUpdateUpdateSupplementDecider : public PublicUpdateSupplementDecider
    {
        time_t m_lastSuccessfulProductUpdate = 0;
    public:
        using PublicUpdateSupplementDecider::PublicUpdateSupplementDecider;

        void setLastSuccessfulProductUpdate(time_t t)
        {
            m_lastSuccessfulProductUpdate = t;
        }
        time_t getLastSuccessfulProductUpdate() override
        {
            return m_lastSuccessfulProductUpdate;
        }

    };

    constexpr int DAYS_IN_WEEK = 7;
    constexpr int SecondsInMin = 60;
    constexpr int SecondsInHour = 60 * SecondsInMin;
    constexpr int SecondsInDay = 24 * SecondsInHour;
    constexpr int SecondsInWeek = 7 * SecondsInDay;
}

TEST_F(TestUpdateSupplementDecider, supplementOnlyAfterProductUpdate) // NOLINT
{
    time_t nowTime = ::time(nullptr);
    std::tm now = *std::localtime(&nowTime);

    UpdateSupplementDecider::WeekDayAndTimeForDelay delay{
        .enabled = true,
        .weekDay = now.tm_wday,
        .hour = now.tm_hour,
        .minute = 0
    };
    FixedLastSuccessfulProductUpdateUpdateSupplementDecider foo(delay);
    foo.setLastSuccessfulProductUpdate(nowTime);
    EXPECT_FALSE(foo.updateProducts());
    EXPECT_FALSE(foo.updateProducts());
}

TEST_F(TestUpdateSupplementDecider, productUpdateIfAWeekHasPassed) // NOLINT
{
    time_t nowTime = ::time(nullptr);
    std::tm now = *std::localtime(&nowTime);

    UpdateSupplementDecider::WeekDayAndTimeForDelay delay{
        .enabled = true,
        .weekDay = now.tm_wday,
        .hour = now.tm_hour,
        .minute = 0
    };
    FixedLastSuccessfulProductUpdateUpdateSupplementDecider foo(delay);
    foo.setLastSuccessfulProductUpdate(nowTime - SecondsInWeek);

    EXPECT_TRUE(foo.updateProducts());
    EXPECT_TRUE(foo.updateProducts());
}


TEST_F(TestUpdateSupplementDecider, productUpdateIfAnHourHasPassed) // NOLINT
{
    time_t nowTime = ::time(nullptr);
    std::tm now = *std::localtime(&nowTime);

    UpdateSupplementDecider::WeekDayAndTimeForDelay delay{
        .enabled = true,
        .weekDay = now.tm_wday,
        .hour = now.tm_hour,
        .minute = 0
    };
    FixedLastSuccessfulProductUpdateUpdateSupplementDecider foo(delay);
    time_t lastUpdate = nowTime - SecondsInHour;
    foo.setLastSuccessfulProductUpdate(lastUpdate);

    time_t lastSchedule = foo.public_lastScheduledProductUpdate();
    EXPECT_LE(lastSchedule, nowTime);
    EXPECT_LE(nowTime - lastSchedule, SecondsInHour);

    EXPECT_TRUE(foo.updateProducts());
    EXPECT_TRUE(foo.updateProducts());
}


TEST_F(TestUpdateSupplementDecider, productUpdateIfAMinuteHasPassed) // NOLINT
{
    time_t nowTime = ::time(nullptr);
    std::tm now = *std::localtime(&nowTime);

    UpdateSupplementDecider::WeekDayAndTimeForDelay delay{
        .enabled = true,
        .weekDay = now.tm_wday,
        .hour = now.tm_hour,
        .minute = now.tm_min
    };
    FixedLastSuccessfulProductUpdateUpdateSupplementDecider foo(delay);
    time_t lastUpdate = nowTime - SecondsInMin;
    foo.setLastSuccessfulProductUpdate(lastUpdate);

    time_t lastSchedule = foo.public_lastScheduledProductUpdate();
    EXPECT_LE(lastSchedule, nowTime);
    EXPECT_LE(nowTime - lastSchedule, SecondsInMin);

    EXPECT_TRUE(foo.updateProducts());
    EXPECT_TRUE(foo.updateProducts());
}

TEST_F(TestUpdateSupplementDecider, supplementOnlyUpdateIfScheduledForTomorrow) // NOLINT
{
    time_t nowTime = ::time(nullptr);
    std::tm now = *std::localtime(&nowTime);

    UpdateSupplementDecider::WeekDayAndTimeForDelay delay{
        .enabled = true,
        .weekDay = (now.tm_wday + 1) % DAYS_IN_WEEK,
        .hour = 8,
        .minute = 0
    };
    FixedLastSuccessfulProductUpdateUpdateSupplementDecider foo(delay);
    time_t lastUpdate = nowTime - SecondsInMin;
    foo.setLastSuccessfulProductUpdate(lastUpdate);

    time_t lastSchedule = foo.public_lastScheduledProductUpdate();
    EXPECT_LE(lastSchedule, nowTime);
    EXPECT_LE(nowTime - lastSchedule, SecondsInWeek);

    EXPECT_FALSE(foo.updateProducts());
}


TEST_F(TestUpdateSupplementDecider, supplementOnlyUpdateIfScheduledForNextHour) // NOLINT
{
    time_t nowTime = ::time(nullptr);
    std::tm now = *std::localtime(&nowTime);

    int weekDay = now.tm_wday;
    int hour = (now.tm_hour + 1) % 24;
    if (hour == 0)
    {
        weekDay = (weekDay + 1) % DAYS_IN_WEEK;
    }

    UpdateSupplementDecider::WeekDayAndTimeForDelay delay{
        .enabled = true,
        .weekDay = weekDay,
        .hour = hour,
        .minute = 0
    };
    FixedLastSuccessfulProductUpdateUpdateSupplementDecider foo(delay);
    time_t lastUpdate = nowTime - SecondsInMin;
    foo.setLastSuccessfulProductUpdate(lastUpdate);

    time_t lastSchedule = foo.public_lastScheduledProductUpdate();
    EXPECT_LE(lastSchedule, nowTime);
    EXPECT_LE(nowTime - lastSchedule, SecondsInWeek);

    EXPECT_FALSE(foo.updateProducts());
}
