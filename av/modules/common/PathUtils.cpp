/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PathUtils.h"

#include <boost/filesystem/path.hpp>

std::string common::PathUtils::lexicallyNormal(const sophos_filesystem::path& p)
{
    auto normalPath = boost::filesystem::path(p).lexically_normal().string();
    return common::PathUtils::removeTrailingDotsFromPath(normalPath);
}

bool common::PathUtils::isNonNormalisedPath(const sophos_filesystem::path& p)
{
    {
        if(contains(p, "//")) return true;

        for (auto& part : boost::filesystem::path(p))
        {
            if (part == ".." || part == ".")
            {
                return true;
            }
        }
        return false;
    }
}
