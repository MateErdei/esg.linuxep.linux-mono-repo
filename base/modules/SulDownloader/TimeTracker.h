/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_TIMETRACKER_H
#define EVEREST_BASE_TIMETRACKER_H

#include <ctime>
#include <string>
namespace SulDownloader
{
    /**
     * Keep track of important time events required by ALC status.
     * https://wiki.sophos.net/display/SophosCloud/EMP%3A+status-alc
     * Proposed Changes for 2018-Q2
     */
    class TimeTracker
    {
    public:
        static std::time_t getCurrTime() ;
        std::string startTime() const;

        std::string finishedTime() const;

        std::string syncTime() const;

        void setStartTime(time_t m_startTime);

        void setFinishedTime(time_t m_finishedTime);

        void setSyncTime(time_t m_syncTime);

    private:
        std::string fromTime(std::time_t) const;
        std::time_t m_startTime = -1;
        std::time_t m_finishedTime = -1;
        std::time_t m_syncTime = -1;

    };
}
#endif //EVEREST_BASE_TIMETRACKER_H
