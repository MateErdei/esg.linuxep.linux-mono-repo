/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "BaseRunner.h"
#include "Logger.h"
#include "Options.h"
#include "ScanCallbackImpl.h"
#include "SigIntMonitor.h"

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class CommandLineScanRunner : public BaseRunner
    {
    public:
        explicit CommandLineScanRunner(const Options& options);
        int run() override;

    private:
        std::vector<std::string> m_paths;
        std::vector<std::string> m_exclusions;
        Logger m_logger;
        std::shared_ptr<SigIntMonitor> m_sigIntMonitor;

        bool m_archiveScanning = false;
        bool m_followSymlinks = false;
    };
}