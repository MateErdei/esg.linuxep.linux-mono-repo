/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PathUtils.h"

#include <boost/filesystem/path.hpp>

std::string common::PathUtils::lexicallyNormal(const sophos_filesystem::path& p)
{
    auto path = boost::filesystem::path(p.string());
    return path.lexically_normal().string();
}
