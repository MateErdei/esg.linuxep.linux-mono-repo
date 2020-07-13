/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "BaseRunner.h"
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
        bool m_help;
        std::vector<std::string> m_paths;
        std::vector<std::string> m_exclusions;
        bool m_archiveScanning;

        int m_returnCode = 0;
    };
}



