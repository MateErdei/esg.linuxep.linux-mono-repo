/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TimeDuration.h"

#include "common/StringUtils.h"
#include "Logger.h"

#include <iostream>

using namespace avscanner::avscannerimpl;
using namespace std;

TimeDuration::TimeDuration(int s, int m, int h)
{
    sec = s;
    min = m;
    hour = h;
}

TimeDuration TimeDuration::timeConversion(const int totalScanTime)
{

    auto newScanTime = int(totalScanTime);

    auto seconds = newScanTime % 60;
    newScanTime /= 60;
    auto minutes = newScanTime % 60;
    newScanTime /= 60;
    auto hours = newScanTime % 24;

    TimeDuration result (seconds,minutes,hours);

    return result;
}

string TimeDuration::toString(const TimeDuration result)
{
    string timingSummary;

    if (result.hour > 0)
    {
        timingSummary += to_string(result.hour) + common::pluralize(result.hour, " hour, ", " hours, ");
    }
    if ((result.min > 0 && result.hour == 0) or result.hour > 0)
    {
        timingSummary += to_string(result.min) + common::pluralize(result.min, " minute, ", " minutes, ");
    }
    if ((result.sec > 0 && result.min == 0) or (result.min > 0 or result.hour > 0))
    {
        timingSummary += to_string(result.sec) + common::pluralize(result.sec, " second.", " seconds.");
    }
    if (result.sec == 0 && result.hour == 0 && result.min == 0)
    {
        timingSummary += "less than a second";
    }

    return  timingSummary;
}