/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <string>
#include "datatypes/sophos_filesystem.h"

namespace common
{
    std::string getPluginVersion();
    sophos_filesystem::path getPluginInstallPath();
}