/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TimeDuration.h"

#include "common/StringUtils.h"
#include "Logger.h"

using namespace avscanner::avscannerimpl;

TimeDuration::TimeDuration(const int totalScanTime)
{
    auto newScanTime = int(totalScanTime);

    m_sec = newScanTime % 60;
    newScanTime /= 60;
    m_minute = newScanTime % 60;
    newScanTime /= 60;
    m_hour = newScanTime % 24;
}

int TimeDuration::getSeconds()
{
    return m_sec;
}

int TimeDuration::getMinutes()
{
    return m_minute;
}

int TimeDuration::getHours()
{
    return m_hour;
}

std::string TimeDuration::toString()
{
    std::string timingSummary;

    if (m_hour > 0)
    {
        timingSummary += std::to_string(m_hour) + common::pluralize(m_hour, " hour, ", " hours, ");
    }
    if ((m_minute > 0 && m_hour == 0) or m_hour > 0)
    {
        timingSummary += std::to_string(m_minute) + common::pluralize(m_minute, " minute, ", " minutes, ");
    }
    if ((m_sec > 0 && m_minute == 0) or (m_minute > 0 or m_hour > 0))
    {
        timingSummary += std::to_string(m_sec) + common::pluralize(m_sec, " second.", " seconds.");
    }
    if (m_sec == 0 && m_hour == 0 && m_minute == 0)
    {
        timingSummary += "less than a second";
    }

    return  timingSummary;
}