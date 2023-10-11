/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/Options.h"
#include <avscanner/avscannerimpl/CommandLineScanRunner.h>
#include <avscanner/avscannerimpl/NamedScanRunner.h>
#include <memory>


using namespace avscanner::avscannerimpl;

int main(int argc, char* argv[])
{
    Options options(argc, argv);
    auto config = options.config();

    std::unique_ptr<IRunner> runner;
    if (config.empty())
    {
        auto paths = options.paths();
        runner = std::make_unique<CommandLineScanRunner>(paths);
    }
    else
    {
        runner = std::make_unique<NamedScanRunner>(config);
    }
    return runner->run();

}
