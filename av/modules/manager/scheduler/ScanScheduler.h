/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once

#include "ScheduledScanConfiguration.h"
#include "ScanRunner.h"

#include "common/AbstractThreadPluginInterface.h"
#include <map>

namespace manager::scheduler
{
    class ScanScheduler : public common::AbstractThreadPluginInterface
    {
    public:
        explicit ScanScheduler(IScanComplete& completionNotifier);
        void run() override;

        /**
         * Update pending config to be config
         * @param config new config
         * @return bool of whether it is valid or not
         */
        bool updateConfig(ScheduledScanConfiguration config);

        void scanNow();

        /**
         * Get how long we should wait before waking up.
         *
         * Based on when the next scheduled scan should run, but a max of 1 hour.
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

        ScheduledScanConfiguration m_pendingConfig;
        std::mutex m_pendingConfigMutex;
        bool m_configIsPending = false;

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

        void updateConfigFromPending();
    };
}
