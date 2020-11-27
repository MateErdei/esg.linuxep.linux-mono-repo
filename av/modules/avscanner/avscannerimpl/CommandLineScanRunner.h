/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "BaseRunner.h"
#include "Logger.h"
#include "Options.h"

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
        bool m_archiveScanning;
        bool m_followSymlinks;

        int m_returnCode = 0;
        Logger m_logger;
    };
}