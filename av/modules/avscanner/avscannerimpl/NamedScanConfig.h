/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <NamedScan.capnp.h>

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class NamedScanConfig
    {
    public:
        explicit NamedScanConfig(const Sophos::ssplav::NamedScan::Reader& namedScanConfig);
        std::string m_scanName;
        std::vector<std::string> m_excludePaths;
        bool m_scanArchives;
    };

    NamedScanConfig configFromFile(const std::string&);
}

