/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "suldownloaderdata/WeekDayAndTimeForDelay.h"

#include <ctime>

namespace SulDownloader
{
    class UpdateSupplementDecider
    {
    public:
        using WeekDayAndTimeForDelay = SulDownloader::suldownloaderdata::WeekDayAndTimeForDelay;
        explicit UpdateSupplementDecider(WeekDayAndTimeForDelay);

        /**
         * @return True if we should update Products or false for supplement-only
         */
        bool updateProducts();

    protected:
        virtual time_t getCurrentTime();
        virtual time_t getLastSuccessfulProductUpdate();
    private:
        WeekDayAndTimeForDelay m_schedule;
    };
}
