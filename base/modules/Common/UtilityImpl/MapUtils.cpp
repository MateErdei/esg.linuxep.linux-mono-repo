/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MapUtils.h"
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

std::map<std::string, std::string> Common::UtilityImpl::getMapFromFile(const std::string & filepath)
{
    std::map<std::string, std::string> map;

    boost::property_tree::ptree ptree;
    try {
        boost::property_tree::read_ini(filepath, ptree);
        for(auto pair : ptree)
        {
            map[pair.first] = pair.second.data();
        }
    }
    catch (boost::property_tree::ini_parser_error & error)
    {
        LOGERROR("File read error on file: " << filepath << " error:" << error.what());
    }

    return map;
}