/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace avscanner::avscannerimpl
{
    class NamedScanRunner
    {
    public:
        explicit NamedScanRunner(const std::string& configPath);
        void run();
    private:
        std::string m_contents;
    };
}



