/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "FakeTimeUtils.h"

AbstractFakeTimeUtils::AbstractFakeTimeUtils(
    std::chrono::milliseconds cacheTimeWindow,
    std::function<void()> reportSimulationEnded) :
    m_cacheTimeWindow{ cacheTimeWindow },
    m_reportSimulationEnded{ reportSimulationEnded },
    m_constructedTime{ std::chrono::system_clock::now() },
    m_lastChangedTime{ m_constructedTime },
    m_latestTime{ .timeLabel = NextTimeStruct::TimeLabel::NOTINITIALIZED, .nextTime = 0 }
{
}

std::time_t AbstractFakeTimeUtils::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_constructedTime);

    if (m_latestTime.timeLabel == NextTimeStruct::TimeLabel::NOTINITIALIZED)
    {
        m_latestTime = getNextTime(elapsed_time);
        m_lastChangedTime = now;
        return m_latestTime.nextTime;
    }

    auto elapsed_time_since_last_change =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastChangedTime);
    if (elapsed_time_since_last_change < m_cacheTimeWindow)
    {
        return m_latestTime.nextTime;
    }

    if (m_latestTime.timeLabel == NextTimeStruct::TimeLabel::LASTONE)
    {
        m_reportSimulationEnded();
        return m_latestTime.nextTime;
    }

    m_latestTime = getNextTime(elapsed_time);
    m_lastChangedTime = now;
    return m_latestTime.nextTime;
}

SequenceOfFakeTime::SequenceOfFakeTime(
    std::vector<std::time_t> times_to_return,
    std::chrono::milliseconds cacheTimeWindow,
    std::function<void()> reportSimulationEnded) :
    AbstractFakeTimeUtils(cacheTimeWindow, reportSimulationEnded),
    m_index{ 0 },
    m_timesToReturn{ times_to_return }
{
}

AbstractFakeTimeUtils::NextTimeStruct SequenceOfFakeTime::getNextTime(
    std::chrono::milliseconds /*durationOfSimulation*/)
{
    size_t nextIndex = m_index + 1;
    size_t currIndex = m_index;
    NextTimeStruct::TimeLabel timeLabel{ NextTimeStruct::TimeLabel::NORMAL };
    if (nextIndex >= m_timesToReturn.size())
    {
        timeLabel = NextTimeStruct::TimeLabel::LASTONE;
    }
    else
    {
        m_index++;
    }

    return { .timeLabel = timeLabel, m_timesToReturn.at(currIndex) };
}
