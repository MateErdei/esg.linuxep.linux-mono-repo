/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

#include <string>

namespace threat_scanner
{
    std::string create_scanner_info(bool scanArchives, bool scanImages);
    sophos_filesystem::path pluginInstall();
}
