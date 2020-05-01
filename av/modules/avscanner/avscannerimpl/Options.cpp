/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <stdexcept>
#include "Options.h"

using namespace avscanner::avscannerimpl;

namespace
{
    bool startswith(const std::string& value, const char* substr)
    {
        return (value.find(substr) == 0);
    }
}

bool Options::handleOption(const std::string& key, const std::string& value)
{
    if (key == "--config")
    {
        m_config = value;
        return true;
    }

    return false;
}

Options::Options(int argc, char** argv)
{
    m_paths.reserve(argc);
    bool allPaths = false;

    for(int i=1; i < argc; i++)
    {
        std::string arg(argv[i]);
        if (allPaths)
        {
            m_paths.emplace_back(arg);
        }
        else if (arg == "--")
        {
            allPaths = true;
        }
        else if (startswith(arg, "--"))
        {
            const char* option = argv[i];
            i++;
            if (i >= argc)
            {
                throw std::runtime_error("Specified an option without a value");
            }
            const char* value = argv[i];
            handleOption(option, value);
        }
        else
        {
            m_paths.emplace_back(arg);
        }
    }
}
