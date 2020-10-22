/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/Main.h>
#include <modules/osqueryextensions/SophosServerTable.cpp>
#include <osquery/flagalias.h>
namespace osquery
{

    FLAG(bool, decorations_top_level, false, "test");
}
using namespace osquery;
static int extension_runner_main(int argc, char* argv[])
{
    osquery::Initializer runner(argc, argv, ToolType::EXTENSION);

    auto status = startExtension("sophos_server_information", "1.0.0");
    if (!status.ok())
    {
        LOG(ERROR) << status.getMessage();
        runner.requestShutdown(status.getCode());
    }

    // Finally wait for a signal / interrupt to shutdown.
    runner.waitThenShutdown();
    return 0;
}

MAIN(extension_runner_main(argc, argv))