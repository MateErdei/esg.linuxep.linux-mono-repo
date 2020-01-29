/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/Options.h"
#include <avscanner/avscannerimpl/CommandLineScanRunner.h>
#include <avscanner/avscannerimpl/NamedScanRunner.h>


using namespace avscanner::avscannerimpl;

int main(int argc, char* argv[])
{
    Options options(argc, argv);
    auto paths = options.paths();
    auto config = options.config();

    if (!config.empty())
    {
        avscanner::avscannerimpl::NamedScanRunner runner(config);
        return runner.run();
    }
    else
    {
        CommandLineScanRunner runner;
        return runner.run(paths);
    }

}
