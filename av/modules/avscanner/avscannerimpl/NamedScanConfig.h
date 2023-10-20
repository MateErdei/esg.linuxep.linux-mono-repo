// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "common/Exclusion.h"

#include "scan_messages/NamedScan.capnp.h"

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class NamedScanConfig
    {
    public:
        explicit NamedScanConfig(const Sophos::ssplav::NamedScan::Reader& namedScanConfig);
        std::string m_scanName;
        std::vector<common::Exclusion> m_excludePaths;
        bool m_scanArchives;
        bool m_scanImages;
        bool m_scanHardDisc;
        bool m_scanOptical;
        bool m_scanNetwork;
        bool m_scanRemovable;
        bool m_detectPUAs;
    };

    NamedScanConfig configFromFile(const std::string&);
}

