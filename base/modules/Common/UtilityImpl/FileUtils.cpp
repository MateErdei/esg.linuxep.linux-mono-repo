/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileUtils.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <string>
#include <Common/FileSystem/IFileSystem.h>

namespace Common::UtilityImpl
{
    std::pair<std::string,std::string> FileUtils::extractValueFromFile(const std::string& filePath, const std::string& key)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(filePath))
        {
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(filePath, ptree);
                return {ptree.get<std::string>(key),""};
            }
            catch (boost::property_tree::ptree_error& ex)
            {
                return {"", ex.what()};
            }
        }
        return {"","File: " + filePath + " does not exist"};
    }
}