/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRunner.h"

#include "NamedScan.capnp.h"

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class NamedScanRunner : public IRunner
    {
    public:
        explicit NamedScanRunner(const std::string& configPath);
        explicit NamedScanRunner(const Sophos::ssplav::NamedScan::Reader& namedScanConfig);
        int run() override;
        std::string getScanName() const;
        std::vector<std::string> getExcludePaths() const;
    private:
        std::string m_scanName;
        std::vector<std::string> m_excludePaths;
    };
}



