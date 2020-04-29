/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "BaseRunner.h"

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class CommandLineScanRunner : public BaseRunner
    {
    public:
        explicit CommandLineScanRunner(std::vector<std::string> paths, bool archiveScanning);
        int run() override;

    private:
        std::vector<std::string> m_paths;
        bool m_archiveScanning;
        int m_returnCode = 0;
    };
}



