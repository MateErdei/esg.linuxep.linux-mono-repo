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

        /*
         * Global config
         */
        std::vector<std::string> exclusions()
        {
            return m_exclusions;
        }

        bool scanAllFileExtensions()
        {
            return m_allFiles;
        }

        bool scanFilesWithNoExtensions()
        {
            return m_scanFilesWithNoExtensions;
        }

        std::vector<std::string> sophosExtensionExclusions()
        {
            return m_sophosExtensionExclusions;
        }


        /*
         * Scans config
         */
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
        std::vector<std::string> m_sophosExtensionExclusions;
        std::vector<ScheduledScan> m_scans;
        ScheduledScan m_scanNowScan;
        bool m_allFiles = false;
        bool m_scanFilesWithNoExtensions = false;
    };
}
