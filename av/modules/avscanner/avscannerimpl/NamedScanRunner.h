/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRunner.h"

#include <string>

namespace avscanner::avscannerimpl
{
    class NamedScanRunner : public IRunner
    {
    public:
        explicit NamedScanRunner(const std::string& configPath);
        int run() override;
    private:
        std::string m_contents;
    };
}



