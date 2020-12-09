/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CheckForTar.h"

#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
    std::vector<std::string> splitPath(const std::string& s)
    {
        const char delim = ':';
        std::vector<std::string> elements;
        std::stringstream ss(s);
        std::string value;

        while (std::getline(ss, value, delim))
        {
            elements.push_back(value);
        }
        return elements;
    }
} // namespace

bool diagnose::CheckForTar::isTarAvailable(const std::string& PATH)
{
    if (PATH.empty())
    {
        return false;
    }
    auto filesystem = Common::FileSystem::fileSystem();
    for (const auto& element : splitPath(PATH))
    {
        auto tar = Common::FileSystem::join(element, "tar");
        if (filesystem->isFile(tar))
        {
            return true;
        }
    }
    return false;
}

bool diagnose::CheckForTar::isTarAvailable()
{
    /**
     * We use ::getenv to match the later execp/system call, which will use the PATH,
     * even if we are called from a setuid binary.
     */
    char* PATH = ::getenv("PATH");
    if (PATH == nullptr)
    {
        LOGERROR("No PATH specified in environment");
        return false;
    }

    std::string path;
    try
    {
        path = Common::UtilityImpl::StringUtils::checkAndConstruct(PATH);
    }
    catch (std::invalid_argument& e)
    {
        LOGERROR("Path is not valid utf8: " << PATH);
        return false;
    }

    return isTarAvailable(path);
}
