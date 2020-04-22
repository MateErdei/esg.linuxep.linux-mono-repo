/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "livequery_main.h"

#include <modules/livequeryimpl/config.h>
#include "modules/livequeryimpl/IQueryProcessor.h"
#include <modules/osqueryclient/OsqueryProcessor.h>
#include <modules/pluginimpl/ApplicationPaths.h>

#include <Common/Logging/PluginLoggingSetup.h>

#include <iostream>

namespace livequery{

    int livequery_main::main(int argc, char* argv[])
    {
        if (argc != 4)
        {
            std::cerr << "Expecting  three parameters got " << (argc - 1) << std::endl;
            return 1;
        }
        Common::Logging::PluginLoggingSetup loggerSetup(PLUGIN_NAME, "livequery");
        std::string correlationid = argv[1];
        std::string query = argv[2];
        std::string socket = argv[3];

        auto queryResponder = livequery::ResponseDispatcher();
        osqueryclient::OsqueryProcessor osqueryProcessor { socket };
        int returnCode = livequery::processQuery(osqueryProcessor, queryResponder, query, correlationid);

        if (returnCode != 0)
        {
            std::cerr << "The query failed to execute with errorcode " << returnCode << std::endl;
            return 3;
        }
        std::cout << queryResponder.getTelemetry();
        return 0;
    }
}
