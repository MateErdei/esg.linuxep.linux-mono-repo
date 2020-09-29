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
        explicit ScanScheduler(IScanComplete& completionNotifier);
        void run() override;

        void updateConfig(ScheduledScanConfiguration);

        void scanNow();

        /**
         * Get how long we should wait before waking up.
         *
         * Based on when the next scheduled scan should run, but a max of 1 hour. (TODO: correct to implementation)
         * (To account for systems being suspended)
         *
         * @param timespec Result of the calculation
         */
        void findNextTime(timespec& timespec);

    protected:
        /**
         * Get the 'current' time, allow to be overridden by unit tests
         * @return
         */
        virtual time_t get_current_time(bool findNextTime);

        /**
         * Get how long to wait before checking, whether to run the next scheduled scan
         * @param next_scheduled_scan_time
         * @param now
         * @return
         */
        virtual time_t calculate_delay(time_t next_scheduled_scan_time, time_t now);

    private:
        IScanComplete& m_completionNotifier;
        Common::Threads::NotifyPipe m_updateConfigurationPipe;
        Common::Threads::NotifyPipe m_scanNowPipe;
        ScheduledScanConfiguration m_config;
        ScheduledScan m_nextScheduledScan;
        time_t m_nextScheduledScanTime;
        using ScanRunnerPtr = std::unique_ptr<ScanRunner>;
        using ScanRunnerMap = std::map<std::string, ScanRunnerPtr>;
        ScanRunnerMap m_runningScans;

        /**
         * Start a thread to run a new scan.
         */
        void runNextScan(const ScheduledScan& nextScan);

        std::string serialiseNextScan(const ScheduledScan& nextScan);
    };
}
