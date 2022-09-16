/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace avscanner::avscannerimpl
{
    class TimeDuration
    {
    public:
        TimeDuration() = delete;
        explicit TimeDuration(int totalScanTime);
        std::string toString();

    private:
        int m_sec = 0;
        int m_minute = 0;
        int m_hour = 0;
    };
}