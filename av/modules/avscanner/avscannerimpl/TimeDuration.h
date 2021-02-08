/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef SSPL_PLUGIN_MAV_TIMEDURATION_H
#define SSPL_PLUGIN_MAV_TIMEDURATION_H

#include <string>

namespace avscanner::avscannerimpl
{
    class TimeDuration
    {
    public:
        int sec, min, hour;
        TimeDuration() = default;
        TimeDuration(int,int,int);
        TimeDuration timeConversion(const int totalScanTime);
        std::string toString(const TimeDuration result);
    };
}

#endif // SSPL_PLUGIN_MAV_TIMEDURATION_H
