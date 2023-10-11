/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScheduledScanConfiguration.h"
#include "ScanRunner.h"

#include <Common/Threads/AbstractThread.h>
#include <map>

namespace manager::scheduler
{
    class ScanScheduler : public Common::Threads::AbstractThread
    {
    public:
        void run() override;

        void updateConfig(ScheduledScanConfiguration);

        void scanNow();

    private:
        Common::Threads::NotifyPipe m_updateConfigurationPipe;
        Common::Threads::NotifyPipe m_scanNowPipe;
        ScheduledScanConfiguration m_config;
        ScheduledScan m_nextScheduledScan;
        time_t m_nextScheduledScanTime;
        using ScanRunnerPtr = std::unique_ptr<ScanRunner>;
        std::map<std::string, ScanRunnerPtr> m_runningScans;

        /**
         * Get how long we should wait before waking up.
         *
         * Based on when the next scheduled scan should run, but a max of 1 hour. (TODO: correct to implementation)
         * (To account for systems being suspended)
         *
         * @param timespec Result of the calculation
         */
        void findNextTime(timespec& timespec);

        /**
         * Start a thread to run a new scan.
         */
        void runNextScan(const ScheduledScan& nextScan);

        std::string serialiseNextScan(const ScheduledScan& nextScan);
    };
}
