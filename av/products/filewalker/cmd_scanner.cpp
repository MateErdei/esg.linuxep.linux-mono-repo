/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "avscanner/avscannerimpl/Options.h"
#include <avscanner/avscannerimpl/CommandLineScanRunner.h>
#include <avscanner/avscannerimpl/NamedScanRunner.h>
#include <datatypes/Print.h>

#include <memory>



using namespace avscanner::avscannerimpl;

int main(int argc, char* argv[])
{
    LogSetup logging;

    Options options(argc, argv);
    auto config = options.config();

    if(options.help())
    {
        PRINT(Options::getHelp());
        return 0;
    }

    std::unique_ptr<IRunner> runner;
    if (config.empty())
    {
        runner = std::make_unique<CommandLineScanRunner>(options);
    }
    else
    {
        runner = std::make_unique<NamedScanRunner>(config);
    }
    return runner->run();
}
