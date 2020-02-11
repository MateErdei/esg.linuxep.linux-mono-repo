/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRunner.h"
#include "NamedScanConfig.h"

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
        NamedScanConfig& getConfig();
    private:
        NamedScanConfig m_config;
    };
}



