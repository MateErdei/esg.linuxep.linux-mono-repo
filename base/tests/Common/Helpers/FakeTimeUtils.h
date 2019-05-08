/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/UtilityImpl/TimeUtils.h>

#include <chrono>
#include <functional>

/**
 * The AbstractFakeTimeUtils helps to create tests that needs to deal with timing and functionality that relies
 * on TimeUtils::getCurrTime
 * It provides two main functionality:
 *   - Calling getCurrentTime multiple times in a cacheTimeWindow will always return the same value, which is useful if
 *     different functions call TimeUtils::getCurrTime and need to 'see' the same view in the 'simulation'.
 *   - It provides a mechanism to inform the test that the simulation is 'done'.
 *
 * Any sort of FakeTime needs only to implement the NextTimeStruct  AbstractFakeTimeUtils::getNextTime(
 * durationOfSimulation)
 *
 */
class AbstractFakeTimeUtils : public Common::UtilityImpl::ITime
{
public:
    struct NextTimeStruct
    {
        enum class TimeLabel
        {
            NOTINITIALIZED,
            NORMAL,
            LASTONE
        };
        TimeLabel timeLabel;
        std::time_t nextTime;
    };
    AbstractFakeTimeUtils(std::chrono::milliseconds cacheTimeWindow, std::function<void()> reportSimulationEnded);
    virtual ~AbstractFakeTimeUtils() = default;
    virtual NextTimeStruct getNextTime(std::chrono::milliseconds durationOfSimulation) = 0;
    std::time_t getCurrentTime() override;

private:
    std::chrono::milliseconds m_cacheTimeWindow;
    std::function<void()> m_reportSimulationEnded;
    std::chrono::system_clock::time_point m_constructedTime;
    std::chrono::system_clock::time_point m_lastChangedTime;
    NextTimeStruct m_latestTime;
};

class SequenceOfFakeTime : public AbstractFakeTimeUtils
{
public:
    SequenceOfFakeTime(
        std::vector<std::time_t> times_to_return,
        std::chrono::milliseconds cacheTimeWindow,
        std::function<void()> reportSimulationEnded);
    NextTimeStruct getNextTime(std::chrono::milliseconds durationOfSimulation) override;

private:
    size_t m_index;
    std::vector<std::time_t> m_timesToReturn;
};
