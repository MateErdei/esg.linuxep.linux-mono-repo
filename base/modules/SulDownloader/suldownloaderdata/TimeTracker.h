// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <ctime>
#include <string>

namespace SulDownloader::suldownloaderdata
{
    /**
     * Keep track of important time events required by ALC status.
     * https://wiki.sophos.net/display/SophosCloud/EMP%3A+status-alc
     * Proposed Changes for 2018-Q2
     */
    class TimeTracker
    {
    public:
        std::string startTime() const;

        std::string finishedTime() const;


        void setStartTime(time_t m_startTime);

        void setFinishedTime(time_t m_finishedTime);


    private:
        std::time_t m_startTime = -1;
        std::time_t m_finishedTime = -1;
    };
} // namespace SulDownloader::suldownloaderdata
