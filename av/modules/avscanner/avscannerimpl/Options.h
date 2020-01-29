/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class Options
    {
    public:
        Options(int argc, char* argv[]);

        [[nodiscard]] std::string config() const
        {
            return m_config;
        }

        [[nodiscard]] std::vector<std::string> paths() const
        {
            return m_paths;
        }

    private:
        std::string m_config;
        std::vector <std::string> m_paths;


        bool handleOption(const std::string& key, const std::string& value);

    };
}

