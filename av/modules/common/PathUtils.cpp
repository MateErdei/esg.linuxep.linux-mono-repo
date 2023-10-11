/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PathUtils.h"


#include <filesystem>
std::string common::PathUtils::lexicallyNormal(const sophos_filesystem::path& p)
{
    auto normalPath = std::filesystem::path(p).lexically_normal().string();
    return common::PathUtils::removeTrailingDotsFromPath(normalPath);
}
