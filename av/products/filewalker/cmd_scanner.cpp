/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "avscanner/avscannerimpl/Options.h"
#include <avscanner/avscannerimpl/CommandLineScanRunner.h>
#include <avscanner/avscannerimpl/NamedScanRunner.h>
#include <memory>


using namespace avscanner::avscannerimpl;

int main(int argc, char* argv[])
{
    LogSetup logging;

    Options options(argc, argv);
    auto config = options.config();

    std::unique_ptr<IRunner> runner;
    if (config.empty())
    {
        auto paths = options.paths();
        runner = std::make_unique<CommandLineScanRunner>(paths, options.archiveScanning());
    }
    else
    {
        runner = std::make_unique<NamedScanRunner>(config);
    }
    return runner->run();
}
