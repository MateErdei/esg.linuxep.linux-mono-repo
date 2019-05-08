/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <tests/Common/Helpers/FakeTimeUtils.h>

#include <thread>

std::time_t t_20190501T13h{ 1556712000 };

TEST(TestFakeTimeUtils, SequenceOfFakeTimeOneEntryOnly) // NOLINT
{
    bool stop{ false };
    SequenceOfFakeTime sequenceOfFakeTime(
        { t_20190501T13h }, std::chrono::milliseconds(10), [&stop]() { stop = true; });
    std::vector<std::time_t> newValues;
    while (!stop)
    {
        auto current_time = sequenceOfFakeTime.getCurrentTime();
        if (newValues.empty())
        {
            newValues.push_back(current_time);
        }
        else
        {
            if (newValues.back() != current_time)
            {
                newValues.push_back(current_time);
            }
        }
    }
    std::vector<std::time_t> expectedValues{ t_20190501T13h };
    EXPECT_EQ(newValues, expectedValues);
}

TEST(TestFakeTimeUtils, SequenceOfFakeTimeTwoEntries) // NOLINT
{
    bool stop{ false };
    std::vector<std::time_t> expectedValues{ t_20190501T13h, t_20190501T13h + 90 };
    SequenceOfFakeTime sequenceOfFakeTime(expectedValues, std::chrono::milliseconds(10), [&stop]() { stop = true; });
    std::vector<std::time_t> newValues;
    while (!stop)
    {
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto current_time = sequenceOfFakeTime.getCurrentTime();
        if (newValues.empty())
        {
            newValues.push_back(current_time);
        }
        else
        {
            if (newValues.back() != current_time)
            {
                newValues.push_back(current_time);
            }
        }
    }

    EXPECT_EQ(newValues, expectedValues);
}

TEST(TestFakeTimeUtils, SequenceOfFakeTimeNEntriesAppliesTheCacheTime) // NOLINT
{
    bool stop{ false };
    std::vector<std::time_t> expectedValues;
    for (int i = 0; i < 10; i++)
    {
        expectedValues.push_back(t_20190501T13h + i * 1000);
    }
    SequenceOfFakeTime sequenceOfFakeTime(expectedValues, std::chrono::milliseconds(10), [&stop]() { stop = true; });
    std::vector<std::time_t> newValues;
    while (!stop)
    {
        auto current_time = sequenceOfFakeTime.getCurrentTime();
        // it does not need to check it the value changed as the amount of time 'sleeping' is bigger than the
        // 'cacheWindowTime'
        newValues.push_back(current_time);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    // there is always one extra call of getCurrentTime when the simulation finished is triggered
    expectedValues.push_back(expectedValues.back());
    EXPECT_EQ(newValues, expectedValues);
}

TEST(TestFakeTimeUtils, DemonstrationHowTheFakeTimeIsToBeHookedIntoTimeUtils) // NOLINT
{
    bool stop{ false };
    std::vector<std::time_t> expectedValues;
    for (int i = 0; i < 10; i++)
    {
        expectedValues.push_back(t_20190501T13h + i * 1000);
    }

    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime{ expectedValues, std::chrono::milliseconds(10), [&stop]() { stop = true; } }));
    std::vector<std::time_t> newValues;
    while (!stop)
    {
        auto current_time = Common::UtilityImpl::TimeUtils::getCurrTime();
        newValues.push_back(current_time);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    // there is always one extra call of getCurrentTime when the simulation finished is triggered
    expectedValues.push_back(expectedValues.back());
    EXPECT_EQ(newValues, expectedValues);
}
