/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/CommandLineScanRunner.h"
#include "datatypes/sophos_filesystem.h"


namespace fs = sophos_filesystem;


TEST(CommandLineScanRunner, construction) // NOLINT
{
    std::vector<std::string> paths;
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths);
}
