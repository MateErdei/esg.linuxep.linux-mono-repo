/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TimeTracker.h"

namespace SulDownloader
{
    std::string TimeTracker::startTime() const
    {
        return fromTime(m_startTime);
    }

    std::string TimeTracker::finishedTime() const
    {
        return fromTime(m_finishedTime);
    }

    std::string TimeTracker::syncTime() const
    {
        if ( m_syncTimeSet)
        {
            return fromTime(m_finishedTime);
        }
        else
        {
            return std::string();
        }
    }

    void TimeTracker::setStartTime(time_t m_startTime)
    {
        TimeTracker::m_startTime = m_startTime;
    }

    void TimeTracker::setFinishedTime(time_t m_finishedTime)
    {
        TimeTracker::m_finishedTime = m_finishedTime;
    }

    std::time_t TimeTracker::getCurrTime()
    {
        return std::time(nullptr);
    }
    std::string TimeTracker::fromTime(std::time_t time_) const
    {
        if ( time_ == -1 )
        {
            return "";
        }

        char formattedTime[16];
        strftime(formattedTime, 16, "%Y%m%d %H%M%S", std::localtime(&time_));

        return formattedTime;
    }

    void TimeTracker::setSyncTime()
    {
        m_syncTimeSet = true;
    }

}
