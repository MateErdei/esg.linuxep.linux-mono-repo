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

        [[nodiscard]] std::string str() const;

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
         * @return True if the scan should look inside archive and image files.
         */
        [[nodiscard]] bool archiveScanning() const
        {
            return m_archiveScanning;
        }

        /**
         * Should this scan cover local non-removable disks?
         * @return
         */
        [[nodiscard]] bool hardDrives() const
        {
            return m_scanLocalFixedDisks;
        }

        /**
         * Should this scan cover local optical disk?
         * @return
         */
        [[nodiscard]] bool CDDVDDrives() const
        {
            return m_scanLocalOpticalDisks;
        }

        /**
         * Should this scan cover remote/network mounts?
         * @return
         */
        [[nodiscard]] bool networkDrives() const
        {
            return m_scanNetworkDrives;
        }

        /**
         * Should this scan cover local removable non-optical disks
         * (Mostly floppy drives)
         * @return
         */
        [[nodiscard]] bool removableDrives() const
        {
            return m_scanRemovableDrives;
        }

    private:
        std::string m_name;
        DaySet m_days;
        TimeSet m_times;
        time_t m_lastRunTime;
        bool m_valid;
        bool m_isScanNow;
        bool m_archiveScanning;
        bool m_scanLocalFixedDisks;
        bool m_scanLocalOpticalDisks;
        bool m_scanNetworkDrives;
        bool m_scanRemovableDrives;
    };
}



