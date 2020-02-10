/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ScheduledScan.h"

namespace manager::scheduler
{
    class ScheduledScanConfiguration
    {
    public:
        ScheduledScanConfiguration() = default;
        explicit ScheduledScanConfiguration(Common::XmlUtilities::AttributesMap& savPolicy);
        std::vector<std::string> exclusions()
        {
            return m_exclusions;
        }
        std::vector<ScheduledScan> scans()
        {
            return m_scans;
        }
        ScheduledScan scanNowScan()
        {
            return m_scanNowScan;
        }
    private:
        std::vector<std::string> m_exclusions;
        std::vector<ScheduledScan> m_scans;
        ScheduledScan m_scanNowScan;
    };
}
