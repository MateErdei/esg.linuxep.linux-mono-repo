//
// Created by pair on 11/06/18.
//

#ifndef EVEREST_BASE_TIMETRACKER_H
#define EVEREST_BASE_TIMETRACKER_H

#include <ctime>

namespace SulDownloader
{
    class TimeTracker
    {
    public:
        static std::time_t getCurrTime() const;
        std::time_t startTime() const;

        std::time_t finishedTime() const;

        std::time_t syncTime() const;

        std::time_t installTime() const;

        void setStartTime(time_t m_startTime);

        void setFinishedTime(time_t m_finishedTime);

        void setSyncTime(time_t m_syncTime);

        void setInstallTime(time_t m_installTime);

    private:
        std::time_t m_startTime;
        std::time_t m_finishedTime;
        std::time_t m_syncTime;
        std::time_t m_installTime;

    };
}
#endif //EVEREST_BASE_TIMETRACKER_H
