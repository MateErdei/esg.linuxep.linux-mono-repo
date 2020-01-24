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
        explicit ScheduledScanConfiguration(Common::XmlUtilities::AttributesMap& savPolicy);
        std::vector<std::string> exclusions()
        {
            return m_exclusions;
        }
        std::vector<ScheduledScan> scans()
        {
            return m_scans;
        }
    private:
        std::vector<std::string> m_exclusions;
        std::vector<ScheduledScan> m_scans;
    };
}
