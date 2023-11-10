// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "TimeTracker.h"

#include "Common/UtilityImpl/TimeUtils.h"

using namespace SulDownloader::suldownloaderdata;
using namespace Common::UtilityImpl;

std::string TimeTracker::startTime() const
{
    return TimeUtils::fromTime(m_startTime);
}

std::string TimeTracker::finishedTime() const
{
    return TimeUtils::fromTime(m_finishedTime);
}



void TimeTracker::setStartTime(time_t m_startTime)
{
    TimeTracker::m_startTime = m_startTime;
}

void TimeTracker::setFinishedTime(time_t m_finishedTime)
{
    TimeTracker::m_finishedTime = m_finishedTime;
}

