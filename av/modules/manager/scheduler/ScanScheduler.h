/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScheduledScanConfiguration.h"

#include <Common/Threads/AbstractThread.h>

namespace manager::scheduler
{
    class ScanScheduler : public Common::Threads::AbstractThread
    {
    public:
        void run() override;
        void updateConfig(ScheduledScanConfiguration);
    private:
        Common::Threads::NotifyPipe m_updateConfigurationPipe;
        ScheduledScanConfiguration m_config;

        /**
         * Get how long we should wait before waking up.
         *
         * Based on when the next scheduled scan should run, but a max of 1 hour. (TODO: correct to implementation)
         * (To account for systems being suspended)
         *
         * @param timespec Result of the calculation
         */
        void findNextTime(timespec& timespec);
    };
}



