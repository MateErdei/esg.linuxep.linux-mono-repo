/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IMountPoint.h"
#include "IRunner.h"
#include "Logger.h"
#include "NamedScanConfig.h"

#include "NamedScan.capnp.h"

#include <memory>
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

        std::vector<std::shared_ptr<IMountPoint>> getIncludedMountpoints(const std::vector<std::shared_ptr<IMountPoint>>& allMountpoints);
    private:
        NamedScanConfig m_config;
        Logger m_logger;
        int m_returnCode = 0;
    };
}



