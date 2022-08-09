/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "common/Poison.h"

#include "BaseRunner.h"
#include "Logger.h"
#include "NamedScan.capnp.h"
#include "NamedScanConfig.h"

#include "mount_monitor/mountinfo/IMountPoint.h"

#include <memory>
#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class NamedScanRunner : public BaseRunner
    {
    public:
        explicit NamedScanRunner(const std::string& configPath);
        explicit NamedScanRunner(const Sophos::ssplav::NamedScan::Reader& namedScanConfig);

        int run() override;

        NamedScanConfig& getConfig();

        /**
         * Public so that we can unit test it.
         *
         * @param allMountpoints
         * @return mount points that we are going to include, based on the config
         */
        [[nodiscard]]
        mount_monitor::mountinfo::IMountPointSharedVector getIncludedMountpoints(const mount_monitor::mountinfo::IMountPointSharedVector& allMountpoints) const;
    private:
        NamedScanConfig m_config;
        Logger m_logger;
    };
}



