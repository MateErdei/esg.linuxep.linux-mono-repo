/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigOptions.h"
#include <Common/FileSystem/IFileSystem.h>
#include <sstream>

namespace MCS
{

    void ConfigOptions::writeToDisk(std::string fullPathToOutFile)
    {
//        auto fs = Common::FileSystem::fileSystem();
        std::stringstream outString;
        for(auto it = config.cbegin(); it != config.cend(); ++it)
        {
            if (!it->second.empty())
            {
                outString << it->first << "=" << it->second << "\n";
            }
        }
        Common::FileSystem::fileSystem()->writeFile(fullPathToOutFile, outString.str());
    }

} // namespace MCS