/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

namespace SulDownloader::suldownloaderdata
{
    struct WeekDayAndTimeForDelay
    {
        int weekDay;
        int hour;
        int minute;
        // seconds is not used when setting up delayed updates
    };
}