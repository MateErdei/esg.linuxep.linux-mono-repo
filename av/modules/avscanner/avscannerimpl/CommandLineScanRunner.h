/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRunner.h"

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class CommandLineScanRunner : public IRunner
    {
    public:
        explicit CommandLineScanRunner(std::vector<std::string> paths);
        int run() override;
    private:
        std::vector<std::string> m_paths;
        int m_returnCode = 0;
    };
}



