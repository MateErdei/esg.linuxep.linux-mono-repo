/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IniFileUtilities.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

namespace Common::UtilityImpl
{
    std::optional<std::string> extractValueFromIniFile(const std::string& filePath, const std::string& key)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(filePath))
        {
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(filePath, ptree);
                return ptree.get<std::string>(key);
            }
            catch (boost::property_tree::ptree_error& ex)
            {
                //LOGWARN("Failed to find key: " << key << " in ini file: " << filePath <<". Error: " << ex.what());
                return std::nullopt;
            }
        }

        //LOGWARN("Could not find ini file to extract data from, file path: " << filePath);
        return std::nullopt;
    }
} // namespace Common::UtilityImpl