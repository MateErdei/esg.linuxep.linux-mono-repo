//
// Created by pair on 11/06/18.
//

#include "TimeTracker.h"

namespace SulDownloader
{
    std::time_t TimeTracker::startTime() const
    {
        return m_startTime;
    }

    std::time_t TimeTracker::finishedTime() const
    {
        return m_finishedTime;
    }

    std::time_t TimeTracker::syncTime() const
    {
        return m_syncTime;
    }

    std::time_t TimeTracker::installTime() const
    {
        return m_installTime;
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

    void TimeTracker::setInstallTime(time_t m_installTime)
    {
        TimeTracker::m_installTime = m_installTime;
    }
}
