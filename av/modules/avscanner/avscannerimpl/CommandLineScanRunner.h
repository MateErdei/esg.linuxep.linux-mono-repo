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
        explicit CommandLineScanRunner(std::vector<std::string> paths);
        int run() override;

    private:
        std::vector<std::string> m_paths;
        int m_returnCode = 0;
    };
}



