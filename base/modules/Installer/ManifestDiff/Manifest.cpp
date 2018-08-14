/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Manifest.h"

#include <fstream>
#include <Common/UtilityImpl/StringUtils.h>

using namespace Installer::ManifestDiff;

Manifest::Manifest(std::istream& file)
{

}

Manifest Manifest::ManifestFromPath(const std::string& filepath)
{
    std::ifstream stream(filepath);
    return Manifest(stream);
}

std::string Manifest::toPosixPath(const std::string& p)
{
    return std::__cxx11::string();
}
