/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

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

        [[nodiscard]] bool archiveScanning() const
        {
            return m_archiveScanning;
        }


    private:
        std::string m_config;
        std::vector <std::string> m_paths;
        bool m_archiveScanning = false;

        static boost::program_options::variables_map parseCommandLine(int argc, char** argv);
    };
}

