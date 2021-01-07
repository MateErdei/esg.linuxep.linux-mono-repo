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
        [[nodiscard]] std::vector<std::string> exclusions() const
        {
            return m_exclusions;
        }

        [[nodiscard]] bool scanAllFileExtensions() const
        {
            return m_allFiles;
        }

        [[nodiscard]] bool scanFilesWithNoExtensions() const
        {
            return m_scanFilesWithNoExtensions;
        }

        [[nodiscard]] std::vector<std::string> sophosExtensionExclusions() const
        {
            return m_sophosExtensionExclusions;
        }

        [[nodiscard]] std::vector<std::string> userDefinedExtensionInclusions() const
        {
            return m_userDefinedExtensionInclusions;
        }


        /*
         * Scans config
         */
        [[nodiscard]] std::vector<ScheduledScan> scans() const
        {
            return m_scans;
        }
        [[nodiscard]] ScheduledScan scanNowScan() const
        {
            return m_scanNowScan;
        }

        [[nodiscard]] bool isValid() const
        {
            return !m_scanNowScan.name().empty();
        }

    private:
        std::vector<std::string> m_exclusions;
        std::vector<std::string> m_sophosExtensionExclusions;
        std::vector<std::string> m_userDefinedExtensionInclusions;
        std::vector<ScheduledScan> m_scans;
        ScheduledScan m_scanNowScan;
        bool m_allFiles = false;
        bool m_scanFilesWithNoExtensions = false;
    };
}
