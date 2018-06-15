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
        return fromTime(m_syncTime);
    }

    void TimeTracker::setStartTime(time_t m_startTime)
    {
        TimeTracker::m_startTime = m_startTime;
    }

    void TimeTracker::setFinishedTime(time_t m_finishedTime)
    {
        TimeTracker::m_finishedTime = m_finishedTime;
    }

    void TimeTracker::setSyncTime(time_t m_syncTime)
    {
        TimeTracker::m_syncTime = m_syncTime;
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

        return std::asctime(std::localtime(&time_)); //FIXME:  expected format 20121125 161343
    }

}
