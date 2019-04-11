/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CheckForTar.h"

#include <Common/FileSystem/IFileSystem.h>

#include <vector>
#include <sstream>

#include <stdlib.h>

namespace
{
    std::vector<std::string> splitPath(const std::string &s)
    {
        const char delim=':';
        std::vector<std::string> elements;
        std::stringstream ss(s);
        std::string value;

        while(std::getline(ss, value, delim))
        {
            elements.push_back(value);
        }
        return elements;
    }
}

bool diagnose::CheckForTar::isTarAvailable(const std::string& PATH)
{
    static_cast<void>(PATH);
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
    return isTarAvailable(PATH);
}
