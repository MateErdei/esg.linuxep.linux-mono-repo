/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "WeekDayAndTimeForDelay.h"

#include <ctime>

namespace SulDownloader::suldownloaderdata
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

        /**
         * @param lastProductUpdateCheck Explicitly provide the lastProductUpdateCheck time
         * @return True if we should update Products or false for supplement-only
         */
        bool updateProducts(time_t lastProductUpdateCheck);

        /**
         * Record that we have successfully updated the product
         */
        static void recordSuccessfulProductUpdate();

    protected:
        virtual time_t getCurrentTime();
        virtual time_t getLastSuccessfulProductUpdate();

        /**
         * Get the time the update should have last been scheduled
         * @return
         */
        time_t lastScheduledProductUpdate();
    private:
        WeekDayAndTimeForDelay m_schedule;
    };
}
