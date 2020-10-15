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
    };

    class FixedLastSuccessfulProductUpdateUpdateSupplementDecider : public UpdateSupplementDecider
    {
        time_t m_lastSuccessfulProductUpdate = 0;
    public:
        using UpdateSupplementDecider::UpdateSupplementDecider;

        void setLastSuccessfulProductUpdate(time_t t)
        {
            m_lastSuccessfulProductUpdate = t;
        }
        time_t getLastSuccessfulProductUpdate() override
        {
            return m_lastSuccessfulProductUpdate;
        }

    };
}

//TEST_F(TestUpdateSupplementDecider, supplementOnlyAfterProductUpdate) // NOLINT
//{
//    time_t nowTime = ::time(nullptr);
//    std::tm now = *std::localtime(&nowTime);
//
//    UpdateSupplementDecider::WeekDayAndTimeForDelay delay{
//        .enabled = true,
//        .weekDay = now.tm_wday,
//        .hour = now.tm_hour,
//        .minute = 0
//    };
//    FixedLastSuccessfulProductUpdateUpdateSupplementDecider foo(delay);
//    foo.setLastSuccessfulProductUpdate(nowTime);
//    EXPECT_FALSE(foo.updateProducts());
//    EXPECT_FALSE(foo.updateProducts());
//}
