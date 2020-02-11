/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "DaySet.h"
#include "TimeSet.h"

namespace manager::scheduler
{
    class ScheduledScan
    {
    public:
        ScheduledScan();

        /**
         * Constructor for a scan now scan
         * @param name name for the scan now scan
         */
        explicit ScheduledScan(std::string name);

        /**
         * Constructor for Standard Scheduled Scan
         * @param savPolicy policy to apply to the scheduled scan
         * @param id scan id for looking up scan settings in the policy
         */
        ScheduledScan(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& id);


        [[nodiscard]] std::string name() const
        {
            return m_name;
        }

        [[nodiscard]] const DaySet& days() const
        {
            return m_days;
        }

        [[nodiscard]] const TimeSet& times() const
        {
            return m_times;
        }

        [[nodiscard]] time_t calculateNextTime(time_t now) const;

        [[nodiscard]] bool valid() const
        {
            return m_valid;
        }

        [[nodiscard]] bool isScanNow() const
        {
            return m_isScanNow;
        }

        /**
         *
         * @return True if the scan should look inside archive files.
         */
        [[nodiscard]] bool archiveScanning() const
        {
            return m_archiveScanning;
        }

    private:
        std::string m_name;
        DaySet m_days;
        TimeSet m_times;
        time_t m_lastRunTime;
        bool m_valid;
        bool m_isScanNow;
        bool m_archiveScanning;
    };
}



