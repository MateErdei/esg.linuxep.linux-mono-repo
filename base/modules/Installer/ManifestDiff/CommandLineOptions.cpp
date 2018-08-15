/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CommandLineOptions.h"

#include <Common/UtilityImpl/StringUtils.h>
#include <iostream>

using namespace Installer::ManifestDiff;

namespace
{
    void split(const std::string& arg, std::string& key, std::string& value)
    {
        auto pos = arg.find('=');
        if (pos == std::string::npos)
        {
            key = arg;
            value = "";
            return;
        }
        key = arg.substr(0,pos);
        value = arg.substr(pos+1);
    }
}

CommandLineOptions::CommandLineOptions(const Common::Datatypes::StringVector& args)
{
    for (size_t i=1; i<args.size(); ++i)
    {
        auto arg = args.at(i);

        std::string key;
        std::string value;
        split(arg,key,value);
        if (key == "--old")
        {
            m_old = value;
        }
        else if (key == "--new")
        {
            m_new = value;
        }
        else if (key == "--diff" || key == "--changed")
        {
            m_changed = value;
        }
        else if (key == "--added")
        {
            m_added = value;
        }
        else if (key == "--removed")
        {
            m_removed = value;
        }
        else
        {
            std::cerr << "Unknown command line option: " << arg << std::endl;
        }
    }
}
