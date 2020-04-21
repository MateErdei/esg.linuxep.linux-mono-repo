/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <iostream>
#include <memory>
#include <modules/osqueryclient/OsqueryProcessor.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <Common/FileSystem/IFileSystem.h>
#include "livequery_main.h"
#include "modules/livequery/IQueryProcessor.h"

namespace livequery{

    int livequery_main::main(int argc, char* argv[])
    {
        std::cout << argc << argv << std::endl;
        if (argc != 4)
        {
            std::cerr << "Expecting  three parameters got " << (argc - 1) << std::endl;
            return 1;
        }

        std::string correlationid  = argv[1];
        std::string query  = argv[2];
        std::string socket  = argv[3];
        auto fileSystem = Common::FileSystem::fileSystem();

        if (!fileSystem->isFile(socket))
        {
            std::cerr << "The socket " << socket << " does not exist" << std::endl;
            return 2;
        }

        livequery::ResponseDispatcher queryResponder();
        osqueryclient::OsqueryProcessor osqueryProcessor { socket };
        livequery::processQuery(osqueryProcessor, reinterpret_cast<IResponseDispatcher &>(queryResponder), query, correlationid);

        return 0;
    }
}
